/*
 * Pane — Paint Implementation
 *
 * Walks the layout tree and generates paint commands in stacking order.
 */

#include "paint.h"
#include <string.h>
#include <stdlib.h>

/* ── Display List Operations ───────────────────────────────────────── */

static void dl_push(DisplayList *dl, PaintCmd cmd)
{
    if (dl->count >= dl->cap) {
        int new_cap = dl->cap ? dl->cap * 2 : 64;
        PaintCmd *new_cmds = arena_alloc(&dl->arena, new_cap * sizeof(PaintCmd), 8);
        if (dl->count > 0)
            memcpy(new_cmds, dl->cmds, dl->count * sizeof(PaintCmd));
        dl->cmds = new_cmds;
        dl->cap = new_cap;
    }
    dl->cmds[dl->count++] = cmd;
}

/* ── Paint a single box ────────────────────────────────────────────── */

static void paint_box(DisplayList *dl, const LayoutBox *box,
                      float offset_x, float offset_y, int z_order)
{
    if (!box->style) return;

    ComputedStyle *s = box->style;
    float x = offset_x + box->rect.x;
    float y = offset_y + box->rect.y;

    /* Background. */
    if (s->background_color.a > 0) {
        Rect border_rect = layout_box_border_rect(box);
        border_rect.x += offset_x;
        border_rect.y += offset_y;

        PaintCmd cmd = {
            .type = PAINT_RECT,
            .rect = border_rect,
            .fill_rect = { .color = s->background_color },
            .z_order = z_order,
        };
        dl_push(dl, cmd);
    }

    /* Borders. */
    if (box->border.top > 0 || box->border.right > 0 ||
        box->border.bottom > 0 || box->border.left > 0) {
        Rect border_rect = layout_box_border_rect(box);
        border_rect.x += offset_x;
        border_rect.y += offset_y;

        PaintCmd cmd = {
            .type = PAINT_BORDER,
            .rect = border_rect,
            .border = {
                .widths = box->border,
                .top_color = s->border_top_color,
                .right_color = s->border_right_color,
                .bottom_color = s->border_bottom_color,
                .left_color = s->border_left_color,
            },
            .z_order = z_order,
        };
        dl_push(dl, cmd);
    }

    /* Text. */
    if (box->type == BOX_TEXT && box->text && box->text_len > 0) {
        PaintCmd cmd = {
            .type = PAINT_TEXT,
            .rect = { x, y, box->rect.width, box->rect.height },
            .text = {
                .text = box->text,
                .len = box->text_len,
                .font_size = s->font_size,
                .color = s->color,
            },
            .z_order = z_order,
        };
        dl_push(dl, cmd);
    }
}

/* ── Recursive paint walk ──────────────────────────────────────────── */

static void paint_tree(DisplayList *dl, const LayoutBox *box,
                       float offset_x, float offset_y, int z_order)
{
    if (!box) return;

    /* Skip invisible boxes. */
    if (box->style && box->style->visibility != VIS_VISIBLE)
        return;

    /* Determine z-order. */
    int z = z_order;
    if (box->style && box->style->z_index.type == VAL_NUMBER) {
        z = (int)box->style->z_index.number;
    }

    /* Paint this box (background + border + text). */
    paint_box(dl, box, offset_x, offset_y, z);

    /* Paint children. */
    float child_offset_x = offset_x + box->rect.x;
    float child_offset_y = offset_y + box->rect.y;

    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        paint_tree(dl, child, child_offset_x, child_offset_y, z);
    }
}

/* ── Sort comparison ───────────────────────────────────────────────── */

static int paint_cmd_cmp(const void *a, const void *b)
{
    const PaintCmd *ca = a;
    const PaintCmd *cb = b;
    return ca->z_order - cb->z_order;
}

/* ── Public API ────────────────────────────────────────────────────── */

DisplayList *paint_generate(const LayoutBox *root)
{
    DisplayList *dl = calloc(1, sizeof(DisplayList));
    arena_init(&dl->arena, 0);
    dl->cap = 64;
    dl->cmds = arena_alloc(&dl->arena, dl->cap * sizeof(PaintCmd), 8);

    paint_tree(dl, root, 0, 0, 0);

    return dl;
}

void paint_sort(DisplayList *dl)
{
    if (dl->count > 1) {
        qsort(dl->cmds, dl->count, sizeof(PaintCmd), paint_cmd_cmp);
    }
}

void paint_free(DisplayList *dl)
{
    if (!dl) return;
    arena_destroy(&dl->arena);
    free(dl);
}
