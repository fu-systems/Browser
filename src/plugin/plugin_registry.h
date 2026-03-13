/*
 * Pane — Plugin Registry
 *
 * Manages plugin lifecycle: register, enable, disable, uninstall.
 * Provides the ordered list of enabled plugins to the pipeline.
 */

#ifndef PANE_PLUGIN_REGISTRY_H
#define PANE_PLUGIN_REGISTRY_H

#include "plugin.h"

#define MAX_PLUGINS 32

/* ── Plugin info (for UI display) ──────────────────────────────────── */

typedef struct {
    const char *name;
    const char *version;
    const char *description;
    const char *author;
    unsigned    capabilities;
    bool        enabled;
} PluginInfo;

/* ── Registry ──────────────────────────────────────────────────────── */

typedef struct {
    PanePlugin *plugins[MAX_PLUGINS];
    int         count;
} PluginRegistry;

/* Create/destroy. */
PluginRegistry *plugin_registry_create(void);
void plugin_registry_free(PluginRegistry *reg);

/* Register a plugin. Takes ownership of the plugin pointer. */
void plugin_registry_register(PluginRegistry *reg, PanePlugin *plugin);

/* Enable/disable by name. */
void plugin_registry_enable(PluginRegistry *reg, const char *name);
void plugin_registry_disable(PluginRegistry *reg, const char *name);

/* Uninstall (disable + remove). */
void plugin_registry_uninstall(PluginRegistry *reg, const char *name);

/* Query. */
bool plugin_registry_is_enabled(const PluginRegistry *reg, const char *name);
PanePlugin *plugin_registry_find(const PluginRegistry *reg, const char *name);

/* Get all registered plugin info (for management UI). */
int plugin_registry_get_info(const PluginRegistry *reg,
                             PluginInfo *out, int max);

#endif /* PANE_PLUGIN_REGISTRY_H */
