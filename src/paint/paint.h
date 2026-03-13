/*
 * Pane — Paint / Display List
 *
 * Converts the positioned layout tree into a flat display list of
 * paint commands. The display list is sorted by stacking order
 * and can be rendered by any backend (terminal, framebuffer, GPU).
 */

#ifndef PANE_PAINT_H
#define PANE_PAINT_H

#include "../layout/box.h"
#include "../css/values.h"
#include "../util/arena.h"

/* ── Paint Command Types ───────────────────────────────────────────── */

typedef enum {
    PAINT_RECT,          /* filled rectangle */
    PAINT_BORDER,        /* box borders */
    PAINT_TEXT,          /* text string */
} PaintCmdType;

/* ── Paint Command ─────────────────────────────────────────────────── */

typedef struct {
    PaintCmdType type;
    Rect         rect;
    union {
        struct {
            CssColor color;
        } fill_rect;
        struct {
            EdgeSizes   widths;
            CssColor    top_color, right_color, bottom_color, left_color;
        } border;
        struct {
            const char *text;
            size_t      len;
            float       font_size;
            CssColor    color;
        } text;
    };
    int z_order;         /* stacking order */
} PaintCmd;

/* ── Display List ──────────────────────────────────────────────────── */

typedef struct {
    PaintCmd *cmds;
    int       count;
    int       cap;
    Arena     arena;
} DisplayList;

/* ── API ────────────────────────────────────────────────────────────── */

/* Generate a display list from a laid-out tree. */
DisplayList *paint_generate(const LayoutBox *root);

/* Free a display list. */
void paint_free(DisplayList *dl);

/* Sort display list by z_order. */
void paint_sort(DisplayList *dl);

#endif /* PANE_PAINT_H */
