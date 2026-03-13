/*
 * Pane — Public API
 *
 * Top-level interface for the Pane rendering engine.
 * Parses HTML + CSS, builds DOM, resolves styles via pairwise context
 * transitions, lays out boxes, and generates a display list.
 *
 * Usage:
 *   PaneResult result = pane_render(html, html_len, css, css_len, 800, 600);
 *   // use result.display_list
 *   pane_result_free(&result);
 */

#ifndef PANE_H
#define PANE_H

#include <stddef.h>
#include "dom/dom.h"
#include "css/css_parser.h"
#include "layout/layout.h"
#include "paint/paint.h"

/* ── Render Result ─────────────────────────────────────────────────── */

typedef struct {
    Document    *document;
    Stylesheet  *stylesheet;
    LayoutTree  *layout_tree;
    DisplayList *display_list;
} PaneResult;

/* ── API ────────────────────────────────────────────────────────────── */

/* Render HTML with optional CSS. Full pipeline:
 *   HTML parse → DOM → CSS cascade → computed style →
 *   pairwise context → layout → paint */
PaneResult pane_render(const char *html, size_t html_len,
                       const char *css, size_t css_len,
                       float viewport_width, float viewport_height);

/* Free all resources. */
void pane_result_free(PaneResult *result);

#endif /* PANE_H */
