/*
 * Pane — Cairo Paint Backend
 *
 * Renders a display list onto a Cairo surface with real font rendering.
 * Also provides direct layout-tree rendering with FreeType fonts.
 */

#ifndef PANE_CAIRO_BACKEND_H
#define PANE_CAIRO_BACKEND_H

#include "paint.h"
#include "../layout/box.h"
#include "../font/font.h"
#include <cairo/cairo.h>

/* ── Font cache for the renderer ───────────────────────────────────── */

typedef struct {
    PaneFont *regular;
    PaneFont *bold;
    PaneFont *italic;
    PaneFont *bold_italic;
    PaneFont *monospace;
    float     base_size;
} FontSet;

/* ── Cairo Renderer ────────────────────────────────────────────────── */

typedef struct {
    cairo_t     *cr;
    FontSet      fonts;
    float        scroll_x, scroll_y;
} CairoRenderer;

/* Initialize a Cairo renderer. Call font_system_init() first. */
void cairo_renderer_init(CairoRenderer *r, cairo_t *cr, float font_size);

/* Destroy renderer (frees fonts). */
void cairo_renderer_destroy(CairoRenderer *r);

/* Render a complete layout tree directly (preferred method). */
void cairo_render_layout(CairoRenderer *r, const LayoutBox *root);

/* Render a display list (alternative method). */
void cairo_render_display_list(CairoRenderer *r, const DisplayList *dl);

/* Clear the surface with a background color. */
void cairo_render_clear(CairoRenderer *r, float bg_r, float bg_g, float bg_b);

#endif /* PANE_CAIRO_BACKEND_H */
