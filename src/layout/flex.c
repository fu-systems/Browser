/*
 * Pane — Flex Layout Implementation
 *
 * Basic flexbox algorithm supporting:
 *   - flex-direction: row, row-reverse, column, column-reverse
 *   - justify-content: flex-start, flex-end, center, space-between, space-around, space-evenly
 *   - align-items: flex-start, flex-end, center, stretch
 *   - flex-grow, flex-shrink
 *   - flex-wrap: nowrap, wrap
 */

#include "flex.h"
#include "block.h"
#include "inline.h"
#include <string.h>
#include <math.h>

/* ── Resolve length to pixels ──────────────────────────────────────── */

static float flex_resolve_len(CssValue val, float font_size, float containing)
{
    switch (val.type) {
    case VAL_LENGTH:
        return css_length_to_px(val, font_size, 16.0f, 0, 0, containing);
    case VAL_PERCENTAGE:
        return val.percentage * containing / 100.0f;
    case VAL_NUMBER:
        return val.number;
    default:
        return 0.0f;
    }
}

/* ── Justify Content Keyword Parsing ───────────────────────────────── */

typedef enum {
    JC_FLEX_START,
    JC_FLEX_END,
    JC_CENTER,
    JC_SPACE_BETWEEN,
    JC_SPACE_AROUND,
    JC_SPACE_EVENLY,
} JustifyContent;

static JustifyContent parse_justify(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string)
        return JC_FLEX_START;
    const char *s = val->string;
    if (strcmp(s, "flex-end") == 0 || strcmp(s, "end") == 0)     return JC_FLEX_END;
    if (strcmp(s, "center") == 0)                                 return JC_CENTER;
    if (strcmp(s, "space-between") == 0)                          return JC_SPACE_BETWEEN;
    if (strcmp(s, "space-around") == 0)                           return JC_SPACE_AROUND;
    if (strcmp(s, "space-evenly") == 0)                           return JC_SPACE_EVENLY;
    return JC_FLEX_START;
}

/* ── Align Items Keyword Parsing ───────────────────────────────────── */

typedef enum {
    AI_STRETCH,
    AI_FLEX_START,
    AI_FLEX_END,
    AI_CENTER,
} AlignItems;

static AlignItems parse_align(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string)
        return AI_STRETCH;
    const char *s = val->string;
    if (strcmp(s, "flex-start") == 0 || strcmp(s, "start") == 0)  return AI_FLEX_START;
    if (strcmp(s, "flex-end") == 0 || strcmp(s, "end") == 0)      return AI_FLEX_END;
    if (strcmp(s, "center") == 0)                                  return AI_CENTER;
    return AI_STRETCH;
}

/* ── Inline type check ─────────────────────────────────────────────── */

static bool flex_is_inline(LayoutBoxType t)
{
    return t == BOX_INLINE || t == BOX_TEXT || t == BOX_INLINE_BLOCK;
}

/* ── Flex Layout ───────────────────────────────────────────────────── */

void layout_flex(LayoutBox *box, float containing_width, float containing_height,
                 Arena *arena)
{
    ComputedStyle *s = box->style;
    if (!s) return;

    float fs = s->font_size;

    /* Resolve box model edges. */
    box->margin = s->margin;
    box->padding = s->padding;
    box->border = s->border_width;

    /* Resolve width. */
    if (s->width.type != VAL_AUTO) {
        float w = flex_resolve_len(s->width, fs, containing_width);
        if (s->box_sizing == BOX_BORDER_BOX) {
            w -= box->padding.left + box->padding.right +
                 box->border.left + box->border.right;
            if (w < 0) w = 0;
        }
        box->rect.width = w;
    } else {
        float w = containing_width
            - box->margin.left - box->margin.right
            - box->border.left - box->border.right
            - box->padding.left - box->padding.right;
        box->rect.width = w > 0 ? w : 0;
    }

    /* Auto margin centering for flex containers. */
    if (s->width.type != VAL_AUTO) {
        float content_w = box->rect.width;
        float remaining = containing_width - content_w
            - box->padding.left - box->padding.right
            - box->border.left - box->border.right;
        if (remaining > 0) {
            if (s->margin_left_auto && s->margin_right_auto) {
                box->margin.left = remaining / 2.0f;
                box->margin.right = remaining / 2.0f;
            } else if (s->margin_left_auto) {
                box->margin.left = remaining - box->margin.right;
            } else if (s->margin_right_auto) {
                box->margin.right = remaining - box->margin.left;
            }
        }
    }

    /* Resolve height (may be auto). */
    float specified_h = -1;
    if (s->height.type != VAL_AUTO) {
        specified_h = flex_resolve_len(s->height, fs, containing_height);
        if (s->box_sizing == BOX_BORDER_BOX) {
            specified_h -= box->padding.top + box->padding.bottom +
                           box->border.top + box->border.bottom;
            if (specified_h < 0) specified_h = 0;
        }
    }

    float content_w = box->rect.width;
    float content_h = specified_h >= 0 ? specified_h : containing_height;

    FlexDirection dir = s->flex_direction;
    bool is_row = (dir == FLEXDIR_ROW || dir == FLEXDIR_ROW_REVERSE);
    bool is_reverse = (dir == FLEXDIR_ROW_REVERSE || dir == FLEXDIR_COLUMN_REVERSE);

    float main_size = is_row ? content_w : content_h;
    float cross_size = is_row ? content_h : content_w;

    JustifyContent jc = parse_justify(&s->justify_content);
    AlignItems ai = parse_align(&s->align_items);

    /* Count visible children and compute their sizes. */
    int child_count = 0;
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;
        child_count++;
    }

    if (child_count == 0) {
        box->rect.height = specified_h >= 0 ? specified_h : 0;
        return;
    }

    /* Lay out each child to determine its natural size. */
    float total_main = 0;
    float max_cross = 0;
    float total_grow = 0;
    float total_shrink = 0;

    /* First pass: layout children at natural size. */
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;

        if (flex_is_inline(c->type)) {
            layout_inline(c, is_row ? content_w : content_h, arena);
        } else {
            layout_block(c, is_row ? content_w : content_h,
                        is_row ? content_h : content_w, arena);
        }

        /* Resolve child's flex-basis. */
        float child_main;
        if (c->style && c->style->flex_basis.type != VAL_AUTO) {
            child_main = flex_resolve_len(c->style->flex_basis, c->style->font_size, main_size);
            if (c->style->box_sizing == BOX_BORDER_BOX) {
                if (is_row) {
                    child_main -= c->padding.left + c->padding.right +
                                  c->border.left + c->border.right;
                } else {
                    child_main -= c->padding.top + c->padding.bottom +
                                  c->border.top + c->border.bottom;
                }
                if (child_main < 0) child_main = 0;
            }
            if (is_row) c->rect.width = child_main;
            else c->rect.height = child_main;
        }

        float child_outer_main, child_outer_cross;
        if (is_row) {
            child_outer_main = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
            child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
        } else {
            child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
        }

        total_main += child_outer_main;
        if (child_outer_cross > max_cross) max_cross = child_outer_cross;

        float grow = c->style ? c->style->flex_grow : 0;
        float shrink = c->style ? c->style->flex_shrink : 1;
        total_grow += grow;
        total_shrink += shrink;
    }

    /* Distribute free space via flex-grow/shrink. */
    float free_space = main_size - total_main;

    if (free_space > 0 && total_grow > 0) {
        /* Grow items. */
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (c->style && c->style->display == DISPLAY_NONE) continue;
            float grow = c->style ? c->style->flex_grow : 0;
            if (grow <= 0) continue;
            float extra = free_space * (grow / total_grow);
            if (is_row) c->rect.width += extra;
            else c->rect.height += extra;
        }
        /* Relayout grown children so their own children reflow. */
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (c->style && c->style->display == DISPLAY_NONE) continue;
            float grow = c->style ? c->style->flex_grow : 0;
            if (grow <= 0) continue;
            if (flex_is_inline(c->type)) {
                layout_inline(c, is_row ? c->rect.width : c->rect.height, arena);
            } else {
                layout_block(c, is_row ? c->rect.width : content_h,
                            is_row ? content_h : c->rect.height, arena);
            }
        }
        free_space = 0;
    } else if (free_space < 0 && total_shrink > 0) {
        /* Shrink items. */
        float deficit = -free_space;
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (c->style && c->style->display == DISPLAY_NONE) continue;
            float shrink = c->style ? c->style->flex_shrink : 1;
            if (shrink <= 0) continue;
            float reduction = deficit * (shrink / total_shrink);
            if (is_row) {
                c->rect.width -= reduction;
                if (c->rect.width < 0) c->rect.width = 0;
            } else {
                c->rect.height -= reduction;
                if (c->rect.height < 0) c->rect.height = 0;
            }
        }
        free_space = 0;
    }

    /* Recompute total_main and max_cross after grow/shrink. */
    total_main = 0;
    max_cross = 0;
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;
        float child_outer_main, child_outer_cross;
        if (is_row) {
            child_outer_main = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
            child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
        } else {
            child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
        }
        total_main += child_outer_main;
        if (child_outer_cross > max_cross) max_cross = child_outer_cross;
    }

    free_space = main_size - total_main;
    if (free_space < 0) free_space = 0;

    /* Position children on main axis using justify-content. */
    float main_pos = 0;
    float gap = 0;

    switch (jc) {
    case JC_FLEX_START:
        main_pos = 0;
        break;
    case JC_FLEX_END:
        main_pos = free_space;
        break;
    case JC_CENTER:
        main_pos = free_space / 2.0f;
        break;
    case JC_SPACE_BETWEEN:
        main_pos = 0;
        gap = child_count > 1 ? free_space / (float)(child_count - 1) : 0;
        break;
    case JC_SPACE_AROUND:
        gap = child_count > 0 ? free_space / (float)child_count : 0;
        main_pos = gap / 2.0f;
        break;
    case JC_SPACE_EVENLY:
        gap = child_count > 0 ? free_space / (float)(child_count + 1) : 0;
        main_pos = gap;
        break;
    }

    if (is_reverse) {
        /* Reverse: start from the end. */
        main_pos = main_size - main_pos;
    }

    /* If height is auto, use max_cross as cross size. */
    float actual_cross = (specified_h >= 0 && is_row) ? specified_h :
                         (!is_row && s->width.type != VAL_AUTO) ? content_w :
                         max_cross;

    /* Position each child. */
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;

        float child_outer_main, child_outer_cross;
        if (is_row) {
            child_outer_main = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
            child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
        } else {
            child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                + c->border.left + c->border.right + c->margin.left + c->margin.right;
        }

        /* Cross-axis alignment. */
        float cross_pos = 0;
        AlignItems item_align = ai;

        /* Check align-self override. */
        if (c->style && c->style->align_self.type == VAL_KEYWORD &&
            c->style->align_self.string) {
            const char *as = c->style->align_self.string;
            if (strcmp(as, "auto") != 0) {
                item_align = parse_align(&c->style->align_self);
            }
        }

        switch (item_align) {
        case AI_FLEX_START:
            cross_pos = 0;
            break;
        case AI_FLEX_END:
            cross_pos = actual_cross - child_outer_cross;
            if (cross_pos < 0) cross_pos = 0;
            break;
        case AI_CENTER:
            cross_pos = (actual_cross - child_outer_cross) / 2.0f;
            if (cross_pos < 0) cross_pos = 0;
            break;
        case AI_STRETCH:
            if (is_row && c->style && c->style->height.type == VAL_AUTO) {
                float stretch_h = actual_cross - c->padding.top - c->padding.bottom
                    - c->border.top - c->border.bottom
                    - c->margin.top - c->margin.bottom;
                if (stretch_h > c->rect.height) c->rect.height = stretch_h;
            } else if (!is_row && c->style && c->style->width.type == VAL_AUTO) {
                float stretch_w = actual_cross - c->padding.left - c->padding.right
                    - c->border.left - c->border.right
                    - c->margin.left - c->margin.right;
                if (stretch_w > c->rect.width) c->rect.width = stretch_w;
            }
            cross_pos = 0;
            break;
        }

        /* Set position. */
        float main_start;
        if (is_reverse) {
            main_start = main_pos - child_outer_main;
        } else {
            main_start = main_pos;
        }

        if (is_row) {
            c->rect.x = main_start + c->margin.left + c->border.left + c->padding.left;
            c->rect.y = cross_pos + c->margin.top + c->border.top + c->padding.top;
        } else {
            c->rect.x = cross_pos + c->margin.left + c->border.left + c->padding.left;
            c->rect.y = main_start + c->margin.top + c->border.top + c->padding.top;
        }

        if (is_reverse) {
            main_pos -= child_outer_main + gap;
        } else {
            main_pos += child_outer_main + gap;
        }
    }

    /* Set container height. */
    if (specified_h >= 0) {
        box->rect.height = specified_h;
    } else if (is_row) {
        box->rect.height = max_cross;
    } else {
        box->rect.height = total_main;
    }

    /* Apply min/max constraints. */
    if (s->min_width.type != VAL_AUTO && s->min_width.type != VAL_NONE) {
        float min_w = flex_resolve_len(s->min_width, fs, containing_width);
        if (box->rect.width < min_w) box->rect.width = min_w;
    }
    if (s->max_width.type != VAL_NONE && s->max_width.type != VAL_AUTO) {
        float max_w = flex_resolve_len(s->max_width, fs, containing_width);
        if (box->rect.width > max_w) box->rect.width = max_w;
    }
    if (s->min_height.type != VAL_AUTO && s->min_height.type != VAL_NONE) {
        float min_h = flex_resolve_len(s->min_height, fs, containing_height);
        if (box->rect.height < min_h) box->rect.height = min_h;
    }
    if (s->max_height.type != VAL_NONE && s->max_height.type != VAL_AUTO) {
        float max_h = flex_resolve_len(s->max_height, fs, containing_height);
        if (box->rect.height > max_h) box->rect.height = max_h;
    }
}
