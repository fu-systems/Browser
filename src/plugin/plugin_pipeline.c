/*
 * Pane — Plugin Pipeline Implementation
 */

#include "plugin_pipeline.h"
#include <stdlib.h>
#include <string.h>

PluginPipeline *plugin_pipeline_create(PluginRegistry *reg,
                                       PluginContext *ctx)
{
    PluginPipeline *pipe = calloc(1, sizeof(PluginPipeline));
    if (!pipe) return NULL;
    pipe->registry = reg;
    pipe->context = ctx;
    return pipe;
}

void plugin_pipeline_free(PluginPipeline *pipe)
{
    /* Does not own registry or context. */
    free(pipe);
}

bool pipeline_run_before_request(PluginPipeline *pipe, FetchRequest *req)
{
    if (!pipe || !req) return true;

    /* Apply context first (spoofed UA, cookies, custom headers). */
    if (pipe->context)
        plugin_context_apply_to_request(pipe->context, req);

    PluginRegistry *reg = pipe->registry;
    for (int i = 0; i < reg->count; i++) {
        PanePlugin *p = reg->plugins[i];
        if (!p->enabled || !p->on_before_request) continue;
        if (!p->on_before_request(p, req))
            return false; /* Plugin cancelled the request. */
    }
    return true;
}

void pipeline_run_after_response(PluginPipeline *pipe, FetchResponse *resp)
{
    if (!pipe || !resp) return;

    PluginRegistry *reg = pipe->registry;
    for (int i = 0; i < reg->count; i++) {
        PanePlugin *p = reg->plugins[i];
        if (!p->enabled || !p->on_after_response) continue;
        p->on_after_response(p, resp);
    }
}

void pipeline_run_dom_ready(PluginPipeline *pipe, Document *doc)
{
    if (!pipe || !doc) return;

    PluginRegistry *reg = pipe->registry;
    for (int i = 0; i < reg->count; i++) {
        PanePlugin *p = reg->plugins[i];
        if (!p->enabled || !p->on_dom_ready) continue;
        p->on_dom_ready(p, doc);
    }
}

bool pipeline_run_navigate(PluginPipeline *pipe,
                           char *url, size_t url_size)
{
    if (!pipe || !url) return true;

    PluginRegistry *reg = pipe->registry;
    for (int i = 0; i < reg->count; i++) {
        PanePlugin *p = reg->plugins[i];
        if (!p->enabled || !p->on_navigate) continue;

        char tmp[4096];
        if (!p->on_navigate(p, url, tmp, sizeof(tmp)))
            return false; /* Plugin cancelled navigation. */
        strncpy(url, tmp, url_size - 1);
        url[url_size - 1] = '\0';
    }
    return true;
}

bool pipeline_run_link_click(PluginPipeline *pipe,
                             const char *href, const char *base_url,
                             char *out_url, size_t out_url_size)
{
    if (!pipe || !href) return true;

    strncpy(out_url, href, out_url_size - 1);
    out_url[out_url_size - 1] = '\0';

    PluginRegistry *reg = pipe->registry;
    for (int i = 0; i < reg->count; i++) {
        PanePlugin *p = reg->plugins[i];
        if (!p->enabled || !p->on_link_click) continue;

        char tmp[4096];
        if (!p->on_link_click(p, out_url, base_url, tmp, sizeof(tmp)))
            return false;
        strncpy(out_url, tmp, out_url_size - 1);
        out_url[out_url_size - 1] = '\0';
    }
    return true;
}
