/*
 * Pane — Layout Box
 *
 * Each DOM element that participates in layout generates a LayoutBox.
 * Boxes form a tree parallel to the DOM, with geometry (x, y, width, height)
 * and the box model edges (margin, border, padding, content).
 */

#ifndef PANE_LAYOUT_BOX_H
#define PANE_LAYOUT_BOX_H

#include "../style/computed.h"
#include "../style/context.h"
#include "../dom/dom.h"
#include "../util/arena.h"

/* ── Box Type ──────────────────────────────────────────────────────── */

typedef enum {
    BOX_BLOCK,
    BOX_INLINE,
    BOX_INLINE_BLOCK,
    BOX_FLEX,
    BOX_GRID,
    BOX_TABLE,
    BOX_TABLE_ROW,
    BOX_TABLE_CELL,
    BOX_TEXT,
    BOX_ANONYMOUS_BLOCK,  /* wrapper for mixed block/inline content */
} LayoutBoxType;

/* ── Geometry ──────────────────────────────────────────────────────── */

typedef struct {
    float x, y;      /* position relative to parent content area */
    float width, height;  /* content area dimensions */
} Rect;

/* ── Layout Box ────────────────────────────────────────────────────── */

typedef struct LayoutBox LayoutBox;
struct LayoutBox {
    LayoutBoxType   type;
    const DomNode  *node;         /* associated DOM node (NULL for anonymous) */
    ComputedStyle  *style;

    /* Box model edges (resolved to pixels). */
    EdgeSizes       margin;
    EdgeSizes       border;
    EdgeSizes       padding;

    /* Content area geometry. */
    Rect            rect;

    /* Tree structure. */
    LayoutBox      *parent;
    LayoutBox      *first_child;
    LayoutBox      *last_child;
    LayoutBox      *next_sibling;
    int             child_count;

    /* For inline/text boxes: text content. */
    const char     *text;
    size_t          text_len;

    /* For text boxes that wrap: width of the last line (< rect.width).
     * Used by the block-level inline formatter to continue inline flow
     * on the last line of a multi-line text box. -1 = not wrapped. */
    float           last_line_width;

    /* Image surface for <img> elements (cairo_surface_t*, owned). */
    void           *image_surface;

    /* Pairwise context for this box's children. */
    PairwiseContext ctx;
};

/* ── API ────────────────────────────────────────────────────────────── */

/* Create a layout box. */
LayoutBox *layout_box_create(Arena *arena, LayoutBoxType type,
                             const DomNode *node, ComputedStyle *style);

/* Append a child box. */
void layout_box_append(LayoutBox *parent, LayoutBox *child);

/* Total outer width/height including margin+border+padding. */
float layout_box_outer_width(const LayoutBox *box);
float layout_box_outer_height(const LayoutBox *box);

/* Margin box rect (position + outer dimensions). */
Rect layout_box_margin_rect(const LayoutBox *box);

/* Border box rect. */
Rect layout_box_border_rect(const LayoutBox *box);

#endif /* PANE_LAYOUT_BOX_H */
