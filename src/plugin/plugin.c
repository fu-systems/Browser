/*
 * Pane — Plugin System Implementation
 */

#include "plugin.h"
#include <stdlib.h>
#include <string.h>
#include "../util/compat.h"
#include <stdio.h>

/* ── FetchRequest ──────────────────────────────────────────────────── */

FetchRequest *fetch_request_create(const char *url)
{
    FetchRequest *req = calloc(1, sizeof(FetchRequest));
    if (!req) return NULL;
    strncpy(req->url, url, sizeof(req->url) - 1);
    strcpy(req->method, "GET");
    return req;
}

void fetch_request_free(FetchRequest *req)
{
    if (!req) return;
    for (int i = 0; i < req->header_count; i++) {
        free(req->header_names[i]);
        free(req->header_values[i]);
    }
    free(req->body);
    free(req);
}

void fetch_request_set_header(FetchRequest *req,
                              const char *name, const char *value)
{
    if (!req || !name || !value) return;

    /* Update existing header. */
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->header_names[i], name) == 0) {
            free(req->header_values[i]);
            req->header_values[i] = strdup(value);
            return;
        }
    }

    /* Add new header. */
    if (req->header_count < FETCH_MAX_HEADERS) {
        char *n = strdup(name);
        char *v = strdup(value);
        if (n && v) {
            req->header_names[req->header_count] = n;
            req->header_values[req->header_count] = v;
            req->header_count++;
        } else {
            free(n);
            free(v);
        }
    }
}

const char *fetch_request_get_header(const FetchRequest *req, const char *name)
{
    if (!req || !name) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->header_names[i], name) == 0)
            return req->header_values[i];
    }
    return NULL;
}

void fetch_request_remove_header(FetchRequest *req, const char *name)
{
    if (!req || !name) return;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->header_names[i], name) == 0) {
            free(req->header_names[i]);
            free(req->header_values[i]);
            /* Shift remaining. */
            for (int j = i; j < req->header_count - 1; j++) {
                req->header_names[j] = req->header_names[j + 1];
                req->header_values[j] = req->header_values[j + 1];
            }
            req->header_count--;
            return;
        }
    }
}

/* ── FetchResponse ─────────────────────────────────────────────────── */

FetchResponse *fetch_response_create(int status_code, const char *body,
                                     size_t body_len, const char *url)
{
    FetchResponse *resp = calloc(1, sizeof(FetchResponse));
    if (!resp) return NULL;
    resp->status_code = status_code;
    if (body && body_len > 0) {
        resp->body = malloc(body_len + 1);
        if (resp->body) {
            memcpy(resp->body, body, body_len);
            resp->body[body_len] = '\0';
            resp->body_len = body_len;
        }
    }
    if (url) resp->url = strdup(url);
    return resp;
}

void fetch_response_free(FetchResponse *resp)
{
    if (!resp) return;
    free(resp->body);
    free(resp->url);
    free(resp->content_type);
    for (int i = 0; i < resp->header_count; i++) {
        free(resp->header_names[i]);
        free(resp->header_values[i]);
    }
    free(resp);
}

void fetch_response_set_header(FetchResponse *resp,
                               const char *name, const char *value)
{
    if (!resp || !name || !value) return;
    for (int i = 0; i < resp->header_count; i++) {
        if (strcasecmp(resp->header_names[i], name) == 0) {
            free(resp->header_values[i]);
            resp->header_values[i] = strdup(value);
            return;
        }
    }
    if (resp->header_count < FETCH_MAX_HEADERS) {
        char *n = strdup(name);
        char *v = strdup(value);
        if (n && v) {
            resp->header_names[resp->header_count] = n;
            resp->header_values[resp->header_count] = v;
            resp->header_count++;
        } else {
            free(n);
            free(v);
        }
    }
}

const char *fetch_response_get_header(const FetchResponse *resp,
                                      const char *name)
{
    if (!resp || !name) return NULL;
    for (int i = 0; i < resp->header_count; i++) {
        if (strcasecmp(resp->header_names[i], name) == 0)
            return resp->header_values[i];
    }
    return NULL;
}

/* ── PluginContext ─────────────────────────────────────────────────── */

PluginContext *plugin_context_create(void)
{
    return calloc(1, sizeof(PluginContext));
}

void plugin_context_free(PluginContext *ctx)
{
    if (!ctx) return;
    for (int i = 0; i < ctx->custom_header_count; i++) {
        free(ctx->custom_header_names[i]);
        free(ctx->custom_header_values[i]);
    }
    free(ctx);
}

void plugin_context_set_custom_header(PluginContext *ctx,
                                      const char *name, const char *value)
{
    if (!ctx || !name || !value) return;
    /* Update existing. */
    for (int i = 0; i < ctx->custom_header_count; i++) {
        if (strcasecmp(ctx->custom_header_names[i], name) == 0) {
            free(ctx->custom_header_values[i]);
            ctx->custom_header_values[i] = strdup(value);
            return;
        }
    }
    if (ctx->custom_header_count < FETCH_MAX_HEADERS) {
        char *n = strdup(name);
        char *v = strdup(value);
        if (n && v) {
            ctx->custom_header_names[ctx->custom_header_count] = n;
            ctx->custom_header_values[ctx->custom_header_count] = v;
            ctx->custom_header_count++;
        } else {
            free(n);
            free(v);
        }
    }
}

void plugin_context_apply_to_request(PluginContext *ctx, FetchRequest *req)
{
    if (!ctx || !req) return;

    /* Spoofed User-Agent. */
    if (ctx->user_agent[0])
        fetch_request_set_header(req, "User-Agent", ctx->user_agent);

    /* Spoofed Accept-Language. */
    if (ctx->accept_language[0])
        fetch_request_set_header(req, "Accept-Language", ctx->accept_language);

    /* Custom headers. */
    for (int i = 0; i < ctx->custom_header_count; i++)
        fetch_request_set_header(req, ctx->custom_header_names[i],
                                      ctx->custom_header_values[i]);

    /* Cookie handling: extract host from URL. */
    const char *host_start = strstr(req->url, "://");
    if (!host_start) return;
    host_start += 3;
    const char *host_end = strchr(host_start, '/');
    size_t host_len = host_end ? (size_t)(host_end - host_start)
                               : strlen(host_start);
    char host[256];
    if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';

    /* Check if domain is blocked. */
    for (int i = 0; i < ctx->blocked_domain_count; i++) {
        if (strcasecmp(host, ctx->blocked_domains[i]) == 0) {
            fetch_request_remove_header(req, "Cookie");
            return;
        }
    }

    /* Inject cookies for matching domains. */
    for (int i = 0; i < ctx->cookie_domain_count; i++) {
        CookieDomain *cd = &ctx->cookie_domains[i];
        if (strcasecmp(host, cd->domain) == 0 ||
            (strlen(host) > strlen(cd->domain) &&
             host[strlen(host) - strlen(cd->domain) - 1] == '.' &&
             strcasecmp(host + strlen(host) - strlen(cd->domain),
                        cd->domain) == 0)) {
            if (cd->count > 0) {
                char cookie_buf[4096] = "";
                size_t pos = 0;
                for (int j = 0; j < cd->count && pos < sizeof(cookie_buf) - 1; j++) {
                    if (j > 0) {
                        pos += snprintf(cookie_buf + pos,
                                        sizeof(cookie_buf) - pos, "; ");
                    }
                    pos += snprintf(cookie_buf + pos,
                                    sizeof(cookie_buf) - pos, "%s=%s",
                                    cd->names[j], cd->values[j]);
                }
                fetch_request_set_header(req, "Cookie", cookie_buf);
            }
            break;
        }
    }
}
