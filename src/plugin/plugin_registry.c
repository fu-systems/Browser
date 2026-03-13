/*
 * Pane — Plugin Registry Implementation
 */

#include "plugin_registry.h"
#include <stdlib.h>
#include <string.h>

PluginRegistry *plugin_registry_create(void)
{
    return calloc(1, sizeof(PluginRegistry));
}

void plugin_registry_free(PluginRegistry *reg)
{
    if (!reg) return;
    for (int i = 0; i < reg->count; i++) {
        if (reg->plugins[i]->enabled && reg->plugins[i]->on_disable)
            reg->plugins[i]->on_disable(reg->plugins[i]);
        if (reg->plugins[i]->on_uninstall)
            reg->plugins[i]->on_uninstall(reg->plugins[i]);
        free(reg->plugins[i]);
    }
    free(reg);
}

void plugin_registry_register(PluginRegistry *reg, PanePlugin *plugin)
{
    if (!reg || !plugin || reg->count >= MAX_PLUGINS) return;

    /* Remove existing plugin with same name. */
    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->plugins[i]->manifest.name,
                   plugin->manifest.name) == 0) {
            if (reg->plugins[i]->enabled && reg->plugins[i]->on_disable)
                reg->plugins[i]->on_disable(reg->plugins[i]);
            free(reg->plugins[i]);
            reg->plugins[i] = plugin;
            return;
        }
    }

    reg->plugins[reg->count++] = plugin;
}

PanePlugin *plugin_registry_find(const PluginRegistry *reg, const char *name)
{
    if (!reg || !name) return NULL;
    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->plugins[i]->manifest.name, name) == 0)
            return reg->plugins[i];
    }
    return NULL;
}

void plugin_registry_enable(PluginRegistry *reg, const char *name)
{
    PanePlugin *p = plugin_registry_find(reg, name);
    if (!p || p->enabled) return;
    p->enabled = true;
    if (p->on_enable) p->on_enable(p);
}

void plugin_registry_disable(PluginRegistry *reg, const char *name)
{
    PanePlugin *p = plugin_registry_find(reg, name);
    if (!p || !p->enabled) return;
    if (p->on_disable) p->on_disable(p);
    p->enabled = false;
}

void plugin_registry_uninstall(PluginRegistry *reg, const char *name)
{
    if (!reg || !name) return;
    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->plugins[i]->manifest.name, name) == 0) {
            PanePlugin *p = reg->plugins[i];
            if (p->enabled) plugin_registry_disable(reg, name);
            if (p->on_uninstall) p->on_uninstall(p);
            free(p);
            /* Shift remaining. */
            for (int j = i; j < reg->count - 1; j++)
                reg->plugins[j] = reg->plugins[j + 1];
            reg->count--;
            return;
        }
    }
}

bool plugin_registry_is_enabled(const PluginRegistry *reg, const char *name)
{
    PanePlugin *p = plugin_registry_find(reg, name);
    return p ? p->enabled : false;
}

int plugin_registry_get_info(const PluginRegistry *reg,
                             PluginInfo *out, int max)
{
    if (!reg || !out) return 0;
    int n = reg->count < max ? reg->count : max;
    for (int i = 0; i < n; i++) {
        PanePlugin *p = reg->plugins[i];
        out[i].name = p->manifest.name;
        out[i].version = p->manifest.version;
        out[i].description = p->manifest.description;
        out[i].author = p->manifest.author;
        out[i].capabilities = p->manifest.capabilities;
        out[i].enabled = p->enabled;
    }
    return n;
}
