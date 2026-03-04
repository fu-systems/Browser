/*
 * Pane — Plugin Pipeline
 *
 * Dispatches hooks to enabled plugins in registration order.
 * Each hook chains: the output of one plugin feeds into the next.
 * For hooks that can cancel (onBeforeRequest, onNavigate, onLinkClick),
 * returning false from any plugin stops the chain.
 */

#ifndef PANE_PLUGIN_PIPELINE_H
#define PANE_PLUGIN_PIPELINE_H

#include "plugin.h"
#include "plugin_registry.h"

typedef struct {
    PluginRegistry *registry;
    PluginContext  *context;
} PluginPipeline;

PluginPipeline *plugin_pipeline_create(PluginRegistry *reg,
                                       PluginContext *ctx);
void plugin_pipeline_free(PluginPipeline *pipe);

/* Network hooks. */

/* Run onBeforeRequest across all enabled plugins.
 * Returns true if the request should proceed, false if cancelled.
 * Context is applied first (spoofed UA, cookies, custom headers). */
bool pipeline_run_before_request(PluginPipeline *pipe, FetchRequest *req);

/* Run onAfterResponse across all enabled plugins. */
void pipeline_run_after_response(PluginPipeline *pipe, FetchResponse *resp);

/* DOM hook. */
void pipeline_run_dom_ready(PluginPipeline *pipe, Document *doc);

/* Navigation hooks.
 * Return true if navigation should proceed (url may be modified in-place).
 * Return false if navigation is cancelled. */
bool pipeline_run_navigate(PluginPipeline *pipe,
                           char *url, size_t url_size);

bool pipeline_run_link_click(PluginPipeline *pipe,
                             const char *href, const char *base_url,
                             char *out_url, size_t out_url_size);

#endif /* PANE_PLUGIN_PIPELINE_H */
