/*
 * Pane — Layout Engine Entry Point
 *
 * Builds the layout tree from DOM + computed styles, runs layout,
 * and produces positioned boxes ready for painting.
 */

#ifndef PANE_LAYOUT_H
#define PANE_LAYOUT_H

#include "box.h"
#include "../dom/dom.h"
#include "../css/css_parser.h"
#include "../util/arena.h"

/* ── Layout Tree ───────────────────────────────────────────────────── */

typedef struct {
    LayoutBox *root;
    Arena      arena;
    float      viewport_width;
    float      viewport_height;
} LayoutTree;

/* ── API ────────────────────────────────────────────────────────────── */

/* Build layout tree and perform layout for the given document.
 * stylesheets: array of parsed stylesheets to apply.
 * ss_count: number of stylesheets. */
LayoutTree *layout_build(Document *doc,
                         Stylesheet **stylesheets, int ss_count,
                         float viewport_width, float viewport_height);

/* Free a layout tree. */
void layout_tree_free(LayoutTree *tree);

#endif /* PANE_LAYOUT_H */
