/*
 * Privacy Shield — built-in plugin that strips tracking parameters
 * and blocks known tracker domains.
 */

#ifndef PANE_PLUGIN_PRIVACY_SHIELD_H
#define PANE_PLUGIN_PRIVACY_SHIELD_H

#include "../plugin.h"

/* Create a new Privacy Shield plugin instance. Caller owns the pointer. */
PanePlugin *privacy_shield_create(void);

#endif /* PANE_PLUGIN_PRIVACY_SHIELD_H */
