/*
 * Privacy Shield — built-in plugin that strips tracking parameters
 * and blocks known tracker domains.
 *
 * Hooks used:
 *   - onBeforeRequest: blocks tracker domains, strips tracking params,
 *     removes Referer header
 *   - onNavigate: strips tracking params from navigation URLs
 */

#include "privacy_shield.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ── Known tracking query-parameter names ──────────────────────────── */

static const char *tracking_params[] = {
    "utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content",
    "fbclid", "gclid", "msclkid", "twclid", "mc_eid", "yclid",
    "ref", "_ga", "dclid", "igshid",
    NULL
};

/* ── Known tracker / analytics domains ─────────────────────────────── */

static const char *blocked_domains[] = {
    "google-analytics.com",
    "googletagmanager.com",
    "facebook.net",
    "doubleclick.net",
    "hotjar.com",
    "segment.io",
    "mixpanel.com",
    NULL
};

/* ── Helpers ───────────────────────────────────────────────────────── */

static bool is_tracking_param(const char *name, size_t len)
{
    for (int i = 0; tracking_params[i]; i++) {
        if (strlen(tracking_params[i]) == len &&
            strncasecmp(name, tracking_params[i], len) == 0)
            return true;
    }
    return false;
}

static bool is_blocked_domain(const char *host)
{
    for (int i = 0; blocked_domains[i]; i++) {
        const char *bd = blocked_domains[i];
        size_t bd_len = strlen(bd);
        size_t h_len = strlen(host);
        if (h_len == bd_len && strcasecmp(host, bd) == 0)
            return true;
        if (h_len > bd_len &&
            host[h_len - bd_len - 1] == '.' &&
            strcasecmp(host + h_len - bd_len, bd) == 0)
            return true;
    }
    return false;
}

/* Extract host from URL into buf. Returns false if parse fails. */
static bool extract_host(const char *url, char *buf, size_t buf_size)
{
    const char *s = strstr(url, "://");
    if (!s) return false;
    s += 3;
    const char *end = strchr(s, '/');
    size_t len = end ? (size_t)(end - s) : strlen(s);
    /* Strip port. */
    const char *colon = memchr(s, ':', len);
    if (colon) len = (size_t)(colon - s);
    if (len >= buf_size) len = buf_size - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return true;
}

/* Strip tracking params from a URL. Returns new URL in out_url. */
static void strip_tracking_params(const char *url, char *out_url, size_t out_size)
{
    /* Find query string. */
    const char *q = strchr(url, '?');
    if (!q) {
        strncpy(out_url, url, out_size - 1);
        out_url[out_size - 1] = '\0';
        return;
    }

    /* Copy base URL. */
    size_t base_len = (size_t)(q - url);
    if (base_len >= out_size) base_len = out_size - 1;
    memcpy(out_url, url, base_len);

    /* Parse and filter query params. */
    const char *fragment = strchr(q, '#');
    const char *end = fragment ? fragment : url + strlen(url);
    const char *p = q + 1;

    size_t pos = base_len;
    bool first = true;

    while (p < end) {
        const char *amp = memchr(p, '&', (size_t)(end - p));
        size_t param_len = amp ? (size_t)(amp - p) : (size_t)(end - p);

        /* Find = separator. */
        const char *eq = memchr(p, '=', param_len);
        size_t name_len = eq ? (size_t)(eq - p) : param_len;

        if (!is_tracking_param(p, name_len)) {
            if (pos < out_size - 1) {
                out_url[pos++] = first ? '?' : '&';
                first = false;
            }
            size_t copy_len = param_len;
            if (pos + copy_len >= out_size) copy_len = out_size - pos - 1;
            memcpy(out_url + pos, p, copy_len);
            pos += copy_len;
        }

        p += param_len;
        if (amp) p++; /* skip '&' */
    }

    /* Append fragment if present. */
    if (fragment) {
        size_t frag_len = strlen(fragment);
        if (pos + frag_len < out_size) {
            memcpy(out_url + pos, fragment, frag_len);
            pos += frag_len;
        }
    }

    out_url[pos] = '\0';
}

/* ── Hook implementations ──────────────────────────────────────────── */

static bool ps_on_before_request(PanePlugin *self, FetchRequest *req)
{
    (void)self;
    char host[256];
    if (!extract_host(req->url, host, sizeof(host)))
        return true;

    /* Block known tracker domains. */
    if (is_blocked_domain(host))
        return false;

    /* Strip tracking query parameters. */
    char clean_url[4096];
    strip_tracking_params(req->url, clean_url, sizeof(clean_url));
    strncpy(req->url, clean_url, sizeof(req->url) - 1);

    /* Strip Referer header. */
    fetch_request_remove_header(req, "Referer");
    fetch_request_remove_header(req, "referer");

    return true;
}

static bool ps_on_navigate(PanePlugin *self, const char *url,
                            char *out_url, size_t out_url_size)
{
    (void)self;
    strip_tracking_params(url, out_url, out_url_size);
    return true;
}

/* ── Create ────────────────────────────────────────────────────────── */

PanePlugin *privacy_shield_create(void)
{
    PanePlugin *p = calloc(1, sizeof(PanePlugin));
    if (!p) return NULL;

    strcpy(p->manifest.name, "privacy_shield");
    strcpy(p->manifest.version, "1.0.0");
    strcpy(p->manifest.description,
           "Strips tracking parameters and blocks tracker domains");
    strcpy(p->manifest.author, "Pane");
    p->manifest.capabilities = PLUGIN_CAP_NETWORK | PLUGIN_CAP_NAVIGATION;

    p->on_before_request = ps_on_before_request;
    p->on_navigate = ps_on_navigate;

    return p;
}
