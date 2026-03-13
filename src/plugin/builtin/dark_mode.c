/*
 * Dark Mode — built-in plugin that injects dark-mode CSS into every page.
 *
 * Works by appending a <style> element to the DOM after parsing,
 * which then flows through the normal style-computation pipeline.
 * Uses !important to override page styles.
 *
 * Hook used: onDomReady
 */

#include "dark_mode.h"
#include <stdlib.h>
#include <string.h>

static const char *DARK_CSS =
    "html, body {\n"
    "  background-color: #1a1a2e !important;\n"
    "  color: #e0e0e0 !important;\n"
    "}\n"
    "a {\n"
    "  color: #7ec8e3 !important;\n"
    "}\n"
    "img {\n"
    "  opacity: 0.85;\n"
    "}\n"
    "* {\n"
    "  border-color: #333355 !important;\n"
    "}\n"
    "input, textarea, select, button {\n"
    "  background-color: #16213e !important;\n"
    "  color: #e0e0e0 !important;\n"
    "  border-color: #0f3460 !important;\n"
    "}\n"
    "table, th, td {\n"
    "  border-color: #333355 !important;\n"
    "}\n"
    "pre, code, kbd, samp {\n"
    "  background-color: #16213e !important;\n"
    "  color: #c8d6e5 !important;\n"
    "}\n";

static void dm_on_dom_ready(PanePlugin *self, Document *doc)
{
    (void)self;
    if (!doc) return;

    /* Create <style> element with dark-mode rules. */
    DomNode *style = doc_create_element(doc, TAG_STYLE, "style");
    DomNode *text = doc_create_text(doc, DARK_CSS, strlen(DARK_CSS));
    dom_append_child(style, text);

    /* Inject into <head> or document element as fallback. */
    if (doc->head) {
        dom_append_child(doc->head, style);
    } else if (doc->html) {
        dom_append_child(doc->html, style);
    }
}

PanePlugin *dark_mode_create(void)
{
    PanePlugin *p = calloc(1, sizeof(PanePlugin));
    if (!p) return NULL;

    strcpy(p->manifest.name, "dark_mode");
    strcpy(p->manifest.version, "1.0.0");
    strcpy(p->manifest.description,
           "Injects dark-mode CSS into every page");
    strcpy(p->manifest.author, "Pane");
    p->manifest.capabilities = PLUGIN_CAP_DOM;

    p->on_dom_ready = dm_on_dom_ready;

    return p;
}
