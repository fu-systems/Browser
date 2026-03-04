/*
 * Cookie Manager — built-in plugin for per-tab cookie control.
 *
 * Hooks used:
 *   - onBeforeRequest: injects Cookie headers for matching domains
 *   - onAfterResponse: parses Set-Cookie headers and stores cookies
 */

#include "cookie_manager.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

/* ── Helpers ───────────────────────────────────────────────────────── */

static bool domain_matches(const char *request_host, const char *cookie_domain)
{
    if (strcasecmp(request_host, cookie_domain) == 0) return true;
    size_t rh_len = strlen(request_host);
    size_t cd_len = strlen(cookie_domain);
    if (rh_len > cd_len &&
        request_host[rh_len - cd_len - 1] == '.' &&
        strcasecmp(request_host + rh_len - cd_len, cookie_domain) == 0)
        return true;
    return false;
}

static bool extract_host(const char *url, char *buf, size_t buf_size)
{
    const char *s = strstr(url, "://");
    if (!s) return false;
    s += 3;
    const char *end = strchr(s, '/');
    size_t len = end ? (size_t)(end - s) : strlen(s);
    const char *colon = memchr(s, ':', len);
    if (colon) len = (size_t)(colon - s);
    if (len >= buf_size) len = buf_size - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return true;
}

static CookieDomain *find_or_create_domain(CookieManagerData *data,
                                            const char *domain)
{
    for (int i = 0; i < data->jar_count; i++) {
        if (strcasecmp(data->jar[i].domain, domain) == 0)
            return &data->jar[i];
    }
    if (data->jar_count >= PLUGIN_MAX_DOMAINS) return NULL;
    CookieDomain *cd = &data->jar[data->jar_count++];
    memset(cd, 0, sizeof(*cd));
    strncpy(cd->domain, domain, sizeof(cd->domain) - 1);
    return cd;
}

static void set_cookie_value(CookieDomain *cd,
                              const char *name, const char *value)
{
    /* Update existing. */
    for (int i = 0; i < cd->count; i++) {
        if (strcmp(cd->names[i], name) == 0) {
            strncpy(cd->values[i], value, sizeof(cd->values[i]) - 1);
            return;
        }
    }
    /* Add new. */
    if (cd->count >= PLUGIN_MAX_COOKIES_PER_DOMAIN) return;
    strncpy(cd->names[cd->count], name, sizeof(cd->names[0]) - 1);
    strncpy(cd->values[cd->count], value, sizeof(cd->values[0]) - 1);
    cd->count++;
}

/* Parse a single Set-Cookie header value. */
static void parse_set_cookie(CookieManagerData *data, const char *raw,
                              const char *request_host)
{
    if (!raw || !raw[0]) return;

    /* The first part before ';' is name=value. */
    const char *semi = strchr(raw, ';');
    size_t nv_len = semi ? (size_t)(semi - raw) : strlen(raw);

    /* Find '=' in name=value. */
    const char *eq = memchr(raw, '=', nv_len);
    if (!eq || eq == raw) return;

    char name[128] = "", value[512] = "";
    size_t name_len = (size_t)(eq - raw);
    if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
    memcpy(name, raw, name_len);
    name[name_len] = '\0';

    /* Trim leading/trailing spaces from name. */
    char *ns = name;
    while (*ns == ' ') ns++;

    const char *vstart = eq + 1;
    size_t val_len = nv_len - (size_t)(vstart - raw);
    if (val_len >= sizeof(value)) val_len = sizeof(value) - 1;
    memcpy(value, vstart, val_len);
    value[val_len] = '\0';

    /* Trim value. */
    char *vs = value;
    while (*vs == ' ') vs++;
    size_t vl = strlen(vs);
    while (vl > 0 && vs[vl - 1] == ' ') vs[--vl] = '\0';

    /* Look for Domain attribute. */
    char domain[256];
    strncpy(domain, request_host, sizeof(domain) - 1);
    domain[sizeof(domain) - 1] = '\0';

    if (semi) {
        const char *p = semi + 1;
        while (*p) {
            while (*p == ' ' || *p == ';') p++;
            if (strncasecmp(p, "domain=", 7) == 0) {
                p += 7;
                if (*p == '.') p++; /* Skip leading dot. */
                const char *dend = strchr(p, ';');
                size_t dlen = dend ? (size_t)(dend - p) : strlen(p);
                if (dlen >= sizeof(domain)) dlen = sizeof(domain) - 1;
                memcpy(domain, p, dlen);
                domain[dlen] = '\0';
                /* Trim. */
                while (dlen > 0 && domain[dlen - 1] == ' ')
                    domain[--dlen] = '\0';
                break;
            }
            const char *next = strchr(p, ';');
            if (!next) break;
            p = next + 1;
        }
    }

    CookieDomain *cd = find_or_create_domain(data, domain);
    if (cd) set_cookie_value(cd, ns, vs);
}

/* ── Hook implementations ──────────────────────────────────────────── */

static bool cm_on_before_request(PanePlugin *self, FetchRequest *req)
{
    CookieManagerData *data = (CookieManagerData *)self->user_data;
    if (!data->cookies_enabled) return true;

    char host[256];
    if (!extract_host(req->url, host, sizeof(host)))
        return true;

    /* Collect cookies that match the request domain. */
    char cookie_buf[4096] = "";
    size_t pos = 0;
    bool has_cookies = false;

    for (int i = 0; i < data->jar_count; i++) {
        if (!domain_matches(host, data->jar[i].domain)) continue;
        CookieDomain *cd = &data->jar[i];
        for (int j = 0; j < cd->count && pos < sizeof(cookie_buf) - 1; j++) {
            if (has_cookies) {
                pos += snprintf(cookie_buf + pos,
                                sizeof(cookie_buf) - pos, "; ");
            }
            pos += snprintf(cookie_buf + pos, sizeof(cookie_buf) - pos,
                            "%s=%s", cd->names[j], cd->values[j]);
            has_cookies = true;
        }
    }

    if (has_cookies)
        fetch_request_set_header(req, "Cookie", cookie_buf);

    return true;
}

static void cm_on_after_response(PanePlugin *self, FetchResponse *resp)
{
    CookieManagerData *data = (CookieManagerData *)self->user_data;
    if (!data->cookies_enabled) return;

    const char *set_cookie = fetch_response_get_header(resp, "set-cookie");
    if (!set_cookie || !set_cookie[0]) return;

    char host[256];
    if (!extract_host(resp->url, host, sizeof(host))) return;

    /* Split on ", " boundaries that look like new cookies. */
    const char *p = set_cookie;
    while (*p) {
        const char *next = strstr(p, ", ");
        if (next) {
            /* Check if the part after ", " looks like a new cookie. */
            const char *after = next + 2;
            const char *eq = strchr(after, '=');
            const char *sc = strchr(after, ';');
            bool looks_like_cookie = eq && eq > after &&
                                     (sc == NULL || eq < sc);
            if (looks_like_cookie) {
                char single[1024];
                size_t len = (size_t)(next - p);
                if (len >= sizeof(single)) len = sizeof(single) - 1;
                memcpy(single, p, len);
                single[len] = '\0';
                parse_set_cookie(data, single, host);
                p = after;
                continue;
            }
        }
        /* No more splits — parse remainder. */
        parse_set_cookie(data, p, host);
        break;
    }

    cookie_manager_save(data);
}

static void cm_on_enable(PanePlugin *self)
{
    CookieManagerData *data = (CookieManagerData *)self->user_data;
    cookie_manager_load(data);
}

/* ── Public API ────────────────────────────────────────────────────── */

CookieManagerData *cookie_manager_get_data(PanePlugin *plugin)
{
    return plugin ? (CookieManagerData *)plugin->user_data : NULL;
}

int cookie_manager_count(CookieManagerData *data)
{
    if (!data) return 0;
    int count = 0;
    for (int i = 0; i < data->jar_count; i++)
        count += data->jar[i].count;
    return count;
}

void cookie_manager_purge(CookieManagerData *data)
{
    if (!data) return;
    data->jar_count = 0;
    memset(data->jar, 0, sizeof(data->jar));
    if (data->storage_path[0]) {
        remove(data->storage_path);
    }
}

void cookie_manager_load(CookieManagerData *data)
{
    if (!data || !data->storage_path[0]) return;

    FILE *f = fopen(data->storage_path, "r");
    if (!f) return;

    /* Simple line-based format: domain\tname\tvalue per line. */
    char line[1024];
    data->jar_count = 0;
    while (fgets(line, sizeof(line), f)) {
        /* Remove newline. */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;

        char *tab1 = strchr(line, '\t');
        if (!tab1) continue;
        *tab1 = '\0';
        char *tab2 = strchr(tab1 + 1, '\t');
        if (!tab2) continue;
        *tab2 = '\0';

        CookieDomain *cd = find_or_create_domain(data, line);
        if (cd) set_cookie_value(cd, tab1 + 1, tab2 + 1);
    }
    fclose(f);
}

void cookie_manager_save(CookieManagerData *data)
{
    if (!data || !data->storage_path[0]) return;

    FILE *f = fopen(data->storage_path, "w");
    if (!f) return;

    for (int i = 0; i < data->jar_count; i++) {
        CookieDomain *cd = &data->jar[i];
        for (int j = 0; j < cd->count; j++) {
            fprintf(f, "%s\t%s\t%s\n", cd->domain,
                    cd->names[j], cd->values[j]);
        }
    }
    fclose(f);
}

/* ── Create ────────────────────────────────────────────────────────── */

PanePlugin *cookie_manager_create(void)
{
    PanePlugin *p = calloc(1, sizeof(PanePlugin));
    if (!p) return NULL;

    CookieManagerData *data = calloc(1, sizeof(CookieManagerData));
    if (!data) { free(p); return NULL; }

    /* Set storage path. */
    const char *home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    if (!home) home = ".";
    snprintf(data->storage_path, sizeof(data->storage_path),
             "%s/.pane_cookies.tsv", home);

    p->user_data = data;

    strcpy(p->manifest.name, "cookie_manager");
    strcpy(p->manifest.version, "1.0.0");
    strcpy(p->manifest.description,
           "Per-tab cookie control with file-based storage");
    strcpy(p->manifest.author, "Pane");
    p->manifest.capabilities = PLUGIN_CAP_NETWORK | PLUGIN_CAP_COOKIES;

    p->on_enable = cm_on_enable;
    p->on_before_request = cm_on_before_request;
    p->on_after_response = cm_on_after_response;

    return p;
}
