/*
 * Pane — Plugin System
 *
 * C port of the original Dart plugin architecture. Provides:
 *   - Plugin base interface with lifecycle + 8 pipeline hooks
 *   - FetchRequest / FetchResponse data objects for network hooks
 *   - PluginManifest for metadata and capability declarations
 *   - PluginContext for site-interaction control (spoofed UA, cookies,
 *     custom headers)
 *
 * Plugins implement a PanePlugin struct with function pointers for the
 * hooks they need. NULL function pointers are treated as pass-through.
 */

#ifndef PANE_PLUGIN_H
#define PANE_PLUGIN_H

#include <stdbool.h>
#include <stddef.h>
#include "../dom/dom.h"
#include "../layout/box.h"

/* ── Capability flags ──────────────────────────────────────────────── */

#define PLUGIN_CAP_NETWORK     (1u << 0)  /* onBeforeRequest, onAfterResponse */
#define PLUGIN_CAP_DOM         (1u << 1)  /* onDomReady */
#define PLUGIN_CAP_STYLE       (1u << 2)  /* onStylesComputed */
#define PLUGIN_CAP_LAYOUT      (1u << 3)  /* onLayoutComplete */
#define PLUGIN_CAP_NAVIGATION  (1u << 4)  /* onNavigate, onLinkClick */
#define PLUGIN_CAP_COOKIES     (1u << 5)  /* cookie control */
#define PLUGIN_CAP_HEADERS     (1u << 6)  /* custom header injection */

/* ── Fetch Request ─────────────────────────────────────────────────── */
/* Mutable request data that plugins can inspect and modify. */

#define FETCH_MAX_HEADERS 64

typedef struct {
    char  url[4096];
    char  method[16];           /* GET, POST, etc. */
    char *header_names[FETCH_MAX_HEADERS];
    char *header_values[FETCH_MAX_HEADERS];
    int   header_count;
    char *body;
    size_t body_len;
} FetchRequest;

FetchRequest *fetch_request_create(const char *url);
void fetch_request_free(FetchRequest *req);
void fetch_request_set_header(FetchRequest *req,
                              const char *name, const char *value);
const char *fetch_request_get_header(const FetchRequest *req, const char *name);
void fetch_request_remove_header(FetchRequest *req, const char *name);

/* ── Fetch Response ────────────────────────────────────────────────── */

typedef struct {
    int     status_code;
    char   *body;
    size_t  body_len;
    char   *url;
    char   *content_type;
    char   *header_names[FETCH_MAX_HEADERS];
    char   *header_values[FETCH_MAX_HEADERS];
    int     header_count;
} FetchResponse;

FetchResponse *fetch_response_create(int status_code, const char *body,
                                     size_t body_len, const char *url);
void fetch_response_free(FetchResponse *resp);
void fetch_response_set_header(FetchResponse *resp,
                               const char *name, const char *value);
const char *fetch_response_get_header(const FetchResponse *resp,
                                      const char *name);

/* ── Plugin Manifest ───────────────────────────────────────────────── */

typedef struct {
    char     name[64];
    char     version[32];
    char     description[256];
    char     author[64];
    unsigned capabilities;      /* bitmask of PLUGIN_CAP_* */
} PluginManifest;

/* ── Plugin Context ────────────────────────────────────────────────── */
/* Shared mutable state for site-interaction control. */

#define PLUGIN_MAX_COOKIES_PER_DOMAIN 64
#define PLUGIN_MAX_DOMAINS 32

typedef struct {
    char domain[256];
    char names[PLUGIN_MAX_COOKIES_PER_DOMAIN][128];
    char values[PLUGIN_MAX_COOKIES_PER_DOMAIN][512];
    int  count;
} CookieDomain;

typedef struct {
    /* Spoofed identity. */
    char user_agent[512];
    char accept_language[128];

    /* Custom request headers. */
    char *custom_header_names[FETCH_MAX_HEADERS];
    char *custom_header_values[FETCH_MAX_HEADERS];
    int   custom_header_count;

    /* Cookie control. */
    CookieDomain cookie_domains[PLUGIN_MAX_DOMAINS];
    int          cookie_domain_count;

    /* Per-domain cookie policy: domain names where cookies are blocked. */
    char blocked_domains[PLUGIN_MAX_DOMAINS][256];
    int  blocked_domain_count;
} PluginContext;

PluginContext *plugin_context_create(void);
void plugin_context_free(PluginContext *ctx);
void plugin_context_apply_to_request(PluginContext *ctx, FetchRequest *req);
void plugin_context_set_custom_header(PluginContext *ctx,
                                      const char *name, const char *value);

/* ── Plugin Interface ──────────────────────────────────────────────── */
/*
 * Each plugin is a PanePlugin struct with function pointers.
 * NULL pointers mean "no-op / pass-through" for that hook.
 * user_data is passed to all callbacks.
 */

typedef struct PanePlugin PanePlugin;

struct PanePlugin {
    PluginManifest manifest;
    void          *user_data;
    bool           enabled;

    /* Lifecycle. */
    void (*on_install)(PanePlugin *self);
    void (*on_enable)(PanePlugin *self);
    void (*on_disable)(PanePlugin *self);
    void (*on_uninstall)(PanePlugin *self);

    /* Network hooks.
     * on_before_request: return true to proceed, false to cancel.
     * The request is modified in-place. */
    bool (*on_before_request)(PanePlugin *self, FetchRequest *req);

    /* on_after_response: modify the response in-place. */
    void (*on_after_response)(PanePlugin *self, FetchResponse *resp);

    /* DOM hook: mutate the document after HTML parsing. */
    void (*on_dom_ready)(PanePlugin *self, Document *doc);

    /* Navigation hooks.
     * Return the (possibly modified) URL in out_url, or return false
     * to cancel navigation. out_url must be at least 4096 bytes. */
    bool (*on_navigate)(PanePlugin *self, const char *url,
                        char *out_url, size_t out_url_size);

    bool (*on_link_click)(PanePlugin *self, const char *href,
                          const char *base_url,
                          char *out_url, size_t out_url_size);
};

#endif /* PANE_PLUGIN_H */
