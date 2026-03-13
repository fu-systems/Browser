/*
 * Dark Mode — built-in plugin that injects dark-mode CSS into every page.
 */

#ifndef PANE_PLUGIN_DARK_MODE_H
#define PANE_PLUGIN_DARK_MODE_H

#include "../plugin.h"

/* Create a new Dark Mode plugin instance. Caller owns the pointer. */
PanePlugin *dark_mode_create(void);

#endif /* PANE_PLUGIN_DARK_MODE_H */
