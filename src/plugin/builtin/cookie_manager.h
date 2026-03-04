/*
 * Cookie Manager — built-in plugin for per-tab cookie control.
 *
 * Parses Set-Cookie response headers, stores cookies in a file-based
 * jar, and injects Cookie headers into requests when enabled.
 */

#ifndef PANE_PLUGIN_COOKIE_MANAGER_H
#define PANE_PLUGIN_COOKIE_MANAGER_H

#include "../plugin.h"

/* Create a new Cookie Manager plugin instance. */
PanePlugin *cookie_manager_create(void);

/* ── Cookie Manager API (accessed via plugin->user_data) ───────────── */

typedef struct {
    /* In-memory cookie jar: array of domain entries. */
    CookieDomain jar[PLUGIN_MAX_DOMAINS];
    int          jar_count;

    /* Whether cookies are enabled for this request cycle.
     * Set by the browser shell before each load based on tab toggle. */
    bool cookies_enabled;

    /* File path for persistent storage. */
    char storage_path[512];
} CookieManagerData;

/* Get the data struct from a cookie_manager plugin. */
CookieManagerData *cookie_manager_get_data(PanePlugin *plugin);

/* Get total cookie count across all domains. */
int cookie_manager_count(CookieManagerData *data);

/* Purge all cookies from memory and disk. */
void cookie_manager_purge(CookieManagerData *data);

/* Load cookies from disk. */
void cookie_manager_load(CookieManagerData *data);

/* Save cookies to disk. */
void cookie_manager_save(CookieManagerData *data);

#endif /* PANE_PLUGIN_COOKIE_MANAGER_H */
