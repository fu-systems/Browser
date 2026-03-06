/*
 * Pane — Block Layout Implementation
 *
 * Block formatting context: children are laid out vertically.
 * Each block-level child occupies the full width of the containing block.
 */

#include "block.h"
#include "inline.h"
#include "flex.h"
#include "grid.h"
#include "table.h"
#include "../dom/dom.h"
#include <math.h>

/* ── Resolve length to pixels ──────────────────────────────────────── */

static float resolve_len(CssValue val, float font_size, float containing)
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

static float resolve_or_zero(CssValue val, float fs, float cb)
{
    if (val.type == VAL_AUTO || val.type == VAL_NONE) return 0;
    return resolve_len(val, fs, cb);
}

/* ── Resolve box model edges ───────────────────────────────────────── */

static void resolve_edges(LayoutBox *box, float containing_width)
{
    ComputedStyle *s = box->style;
    float fs = s->font_size;

    box->margin = s->margin;
    box->padding = s->padding;
    box->border = s->border_width;

    /* Margin: auto handling for block horizontal centering. */
    if (box->type == BOX_BLOCK && s->width.type != VAL_AUTO) {
        float content_w = resolve_len(s->width, fs, containing_width);
        if (s->box_sizing == BOX_BORDER_BOX) {
            content_w -= box->padding.left + box->padding.right
                       + box->border.left + box->border.right;
            if (content_w < 0) content_w = 0;
        }
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
}

/* ── Width Resolution ──────────────────────────────────────────────── */

static float resolve_width(LayoutBox *box, float containing_width)
{
    ComputedStyle *s = box->style;
    float fs = s->font_size;

    if (s->width.type != VAL_AUTO) {
        float w = resolve_len(s->width, fs, containing_width);
        if (s->box_sizing == BOX_BORDER_BOX) {
            w -= box->padding.left + box->padding.right +
                 box->border.left + box->border.right;
            if (w < 0) w = 0;
        }
        return w;
    }

    /* Auto width: fill containing block. */
    float w = containing_width
        - box->margin.left - box->margin.right
        - box->border.left - box->border.right
        - box->padding.left - box->padding.right;
    return w > 0 ? w : 0;
}

/* ── Height Resolution ─────────────────────────────────────────────── */

static float resolve_height(LayoutBox *box, float containing_height)
{
    ComputedStyle *s = box->style;
    float fs = s->font_size;

    if (s->height.type != VAL_AUTO) {
        float h = resolve_len(s->height, fs, containing_height);
        if (s->box_sizing == BOX_BORDER_BOX) {
            h -= box->padding.top + box->padding.bottom +
                 box->border.top + box->border.bottom;
            if (h < 0) h = 0;
        }
        return h;
    }

    return -1; /* auto: determined by content */
}

/* Forward declaration. */
static void layout_children(LayoutBox *box, Arena *arena);

/* ── Block Layout ──────────────────────────────────────────────────── */

void layout_block(LayoutBox *box, float containing_width, float containing_height,
                  Arena *arena)
{
    resolve_edges(box, containing_width);

    /* Resolve width. */
    box->rect.width = resolve_width(box, containing_width);

    /* Lay out children. */
    layout_children(box, arena);

    /* Resolve height. */
    float specified_h = resolve_height(box, containing_height);
    if (specified_h >= 0) {
        box->rect.height = specified_h;
    }
    /* Otherwise height was set by layout_children. */

    /* Apply min/max constraints. */
    ComputedStyle *s = box->style;
    float fs = s->font_size;
    if (s->min_width.type != VAL_AUTO && s->min_width.type != VAL_NONE) {
        float min_w = resolve_len(s->min_width, fs, containing_width);
        if (box->rect.width < min_w) box->rect.width = min_w;
    }
    if (s->max_width.type != VAL_NONE && s->max_width.type != VAL_AUTO) {
        float max_w = resolve_len(s->max_width, fs, containing_width);
        if (box->rect.width > max_w) box->rect.width = max_w;
    }
    if (s->min_height.type != VAL_AUTO && s->min_height.type != VAL_NONE) {
        float min_h = resolve_len(s->min_height, fs, containing_height);
        if (box->rect.height < min_h) box->rect.height = min_h;
    }
    if (s->max_height.type != VAL_NONE && s->max_height.type != VAL_AUTO) {
        float max_h = resolve_len(s->max_height, fs, containing_height);
        if (box->rect.height > max_h) box->rect.height = max_h;
    }
}

/* ── Margin Collapsing ─────────────────────────────────────────────── */

static float collapse_margins(float margin_a, float margin_b)
{
    /* Both positive: larger wins. Both negative: more negative wins.
     * One each: sum them. */
    if (margin_a >= 0 && margin_b >= 0)
        return fmaxf(margin_a, margin_b);
    if (margin_a < 0 && margin_b < 0)
        return fminf(margin_a, margin_b);
    return margin_a + margin_b;
}

/* ── Position Relative Offsets ──────────────────────────────────────── */

static void apply_relative_offset(LayoutBox *child, float containing_w, float containing_h)
{
    if (!child->style || child->style->position != POSITION_RELATIVE) return;
    float fs = child->style->font_size;
    if (child->style->top.type != VAL_AUTO)
        child->rect.y += resolve_len(child->style->top, fs, containing_h);
    else if (child->style->bottom.type != VAL_AUTO)
        child->rect.y -= resolve_len(child->style->bottom, fs, containing_h);
    if (child->style->left.type != VAL_AUTO)
        child->rect.x += resolve_len(child->style->left, fs, containing_w);
    else if (child->style->right.type != VAL_AUTO)
        child->rect.x -= resolve_len(child->style->right, fs, containing_w);
}

/* ── Child Layout ──────────────────────────────────────────────────── */

static bool is_inline_type(LayoutBoxType t)
{
    return t == BOX_INLINE || t == BOX_TEXT || t == BOX_INLINE_BLOCK;
}

static void layout_children(LayoutBox *box, Arena *arena)
{
    float cursor_y = 0;
    float prev_margin_bottom = 0;
    float content_width = box->rect.width;
    float float_max_bottom = 0;  /* Track the bottom of floated elements. */
    float float_left_bottom = 0;   /* Bottom of left floats. */
    float float_right_bottom = 0;  /* Bottom of right floats. */

    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        if (child->style && child->style->display == DISPLAY_NONE)
            continue;

        /* Absolutely/fixed positioned elements are out of normal flow.
         * Lay them out but don't advance cursor_y. */
        if (child->style && (child->style->position == POSITION_ABSOLUTE ||
                             child->style->position == POSITION_FIXED)) {
            layout_block(child, content_width, box->rect.height, arena);

            /* Position using top/left if specified, otherwise at current cursor. */
            float abs_x = 0, abs_y = 0;
            float fs = child->style->font_size;
            if (child->style->left.type != VAL_AUTO)
                abs_x = resolve_len(child->style->left, fs, content_width);
            if (child->style->top.type != VAL_AUTO)
                abs_y = resolve_len(child->style->top, fs, box->rect.height);
            if (child->style->right.type != VAL_AUTO && child->style->left.type == VAL_AUTO) {
                float r_val = resolve_len(child->style->right, fs, content_width);
                abs_x = content_width - child->rect.width
                      - child->padding.left - child->padding.right
                      - child->border.left - child->border.right
                      - child->margin.left - child->margin.right - r_val;
            }
            if (child->style->bottom.type != VAL_AUTO && child->style->top.type == VAL_AUTO) {
                float b_val = resolve_len(child->style->bottom, fs, box->rect.height);
                abs_y = box->rect.height - child->rect.height
                      - child->padding.top - child->padding.bottom
                      - child->border.top - child->border.bottom
                      - child->margin.top - child->margin.bottom - b_val;
            }

            child->rect.x = abs_x + child->margin.left + child->border.left + child->padding.left;
            child->rect.y = abs_y + child->margin.top + child->border.top + child->padding.top;
            continue;
        }

        /* Floated elements: position at left/right edge, out of normal flow. */
        if (child->style && child->style->float_val != FLOAT_NONE) {
            layout_block(child, content_width, box->rect.height, arena);
            float outer_w = layout_box_outer_width(child);

            if (child->style->float_val == FLOAT_RIGHT) {
                child->rect.x = content_width - outer_w +
                                child->margin.left + child->border.left + child->padding.left;
            } else {
                child->rect.x = child->margin.left + child->border.left + child->padding.left;
            }
            child->rect.y = cursor_y + child->margin.top + child->border.top + child->padding.top;

            /* Track float extent for auto height calculation and clear. */
            float float_bottom = child->rect.y + child->rect.height +
                                 child->padding.bottom + child->border.bottom +
                                 child->margin.bottom;
            if (float_bottom > float_max_bottom)
                float_max_bottom = float_bottom;
            if (child->style->float_val == FLOAT_LEFT && float_bottom > float_left_bottom)
                float_left_bottom = float_bottom;
            if (child->style->float_val == FLOAT_RIGHT && float_bottom > float_right_bottom)
                float_right_bottom = float_bottom;
            apply_relative_offset(child, content_width, box->rect.height);
            continue;
        }

        /* Handle CSS clear property. */
        if (child->style && child->style->clear_val != CLEAR_NONE) {
            float clear_to = 0;
            if (child->style->clear_val == CLEAR_LEFT || child->style->clear_val == CLEAR_BOTH) {
                if (float_left_bottom > clear_to) clear_to = float_left_bottom;
            }
            if (child->style->clear_val == CLEAR_RIGHT || child->style->clear_val == CLEAR_BOTH) {
                if (float_right_bottom > clear_to) clear_to = float_right_bottom;
            }
            if (clear_to > cursor_y) cursor_y = clear_to;
        }

        if (is_inline_type(child->type)) {
            /* ── Inline formatting context: flow consecutive inline children
             *    horizontally with word-wrapping. ─────────────────────── */
            float x = 0;
            float line_h = 0;
            float font_size = box->style ? box->style->font_size : 16.0f;
            float default_line_h = font_size * (box->style ? box->style->line_height : 1.2f);
            TextAlign align = box->style ? box->style->text_align : TEXT_ALIGN_START;
            bool no_wrap = box->style && (box->style->white_space == WS_NOWRAP ||
                                           box->style->white_space == WS_PRE);

            /* Track line starts for text-align adjustment. */
            LayoutBox *line_start = NULL;
            float line_start_y = cursor_y;

            for (; child; child = child->next_sibling) {
                if (child->style && child->style->display == DISPLAY_NONE)
                    continue;
                if (!is_inline_type(child->type))
                    break;

                /* Handle <br>: force a line break. */
                if (child->node && child->node->type == PANE_NODE_ELEMENT &&
                    child->node->elem.tag == TAG_BR) {
                    /* Apply text-align to the current line before breaking. */
                    if ((align == TEXT_ALIGN_CENTER || align == TEXT_ALIGN_RIGHT ||
                         align == TEXT_ALIGN_END) && line_start) {
                        float offset = content_width - x;
                        if (align == TEXT_ALIGN_CENTER) offset /= 2.0f;
                        if (offset > 0) {
                            for (LayoutBox *lc = line_start; lc && lc != child;
                                 lc = lc->next_sibling) {
                                if (lc->style && lc->style->display == DISPLAY_NONE)
                                    continue;
                                lc->rect.x += offset;
                            }
                        }
                    }
                    cursor_y += line_h > 0 ? line_h : default_line_h;
                    x = 0;
                    line_h = 0;
                    line_start = NULL;
                    child->rect.width = 0;
                    child->rect.height = 0;
                    continue;
                }

                layout_inline(child, content_width, arena);

                float child_outer_w = child->rect.width + child->padding.left
                    + child->padding.right + child->border.left + child->border.right
                    + child->margin.left + child->margin.right;
                float child_outer_h = child->rect.height + child->padding.top
                    + child->padding.bottom + child->border.top + child->border.bottom
                    + child->margin.top + child->margin.bottom;

                /* Wrap to next line if needed (unless nowrap). */
                if (!no_wrap && x + child_outer_w > content_width && x > 0) {
                    /* Apply text-align to completed line. */
                    if (align == TEXT_ALIGN_CENTER || align == TEXT_ALIGN_RIGHT ||
                        align == TEXT_ALIGN_END) {
                        float offset = content_width - x;
                        if (align == TEXT_ALIGN_CENTER) offset /= 2.0f;
                        if (offset > 0) {
                            for (LayoutBox *lc = line_start; lc && lc != child;
                                 lc = lc->next_sibling) {
                                if (lc->style && lc->style->display == DISPLAY_NONE)
                                    continue;
                                lc->rect.x += offset;
                            }
                        }
                    }
                    cursor_y += line_h > 0 ? line_h : default_line_h;
                    x = 0;
                    line_h = 0;
                    line_start = child;
                    line_start_y = cursor_y;
                }

                if (!line_start) line_start = child;

                child->rect.x = x + child->margin.left + child->border.left + child->padding.left;
                child->rect.y = cursor_y + child->margin.top + child->border.top + child->padding.top;
                x += child_outer_w;
                if (child_outer_h > line_h) line_h = child_outer_h;
            }

            /* Apply text-align to the last line. */
            if (align == TEXT_ALIGN_CENTER || align == TEXT_ALIGN_RIGHT ||
                align == TEXT_ALIGN_END) {
                float offset = content_width - x;
                if (align == TEXT_ALIGN_CENTER) offset /= 2.0f;
                if (offset > 0 && line_start) {
                    for (LayoutBox *lc = line_start; lc; lc = lc->next_sibling) {
                        if (lc->style && lc->style->display == DISPLAY_NONE)
                            continue;
                        if (!is_inline_type(lc->type)) break;
                        lc->rect.x += offset;
                    }
                }
            }

            /* Close the last line. */
            cursor_y += line_h > 0 ? line_h : default_line_h;
            prev_margin_bottom = 0;

            /* The for loop advanced child past the last inline;
             * check if we need to continue with the current (block) child. */
            if (!child) break;
            /* Fall through to handle the current non-inline child below. */
        }

        switch (child->type) {
        case BOX_FLEX: {
            layout_flex(child, content_width, box->rect.height, arena);

            /* Margin collapsing: collapse top margin with previous bottom. */
            float collapsed = collapse_margins(prev_margin_bottom, child->margin.top);

            child->rect.x = child->margin.left + child->border.left + child->padding.left;
            child->rect.y = cursor_y + collapsed + child->border.top + child->padding.top;

            cursor_y = child->rect.y + child->rect.height +
                       child->padding.bottom + child->border.bottom;
            prev_margin_bottom = child->margin.bottom;
            apply_relative_offset(child, content_width, box->rect.height);
            break;
        }

        case BOX_TABLE: {
            layout_table(child, content_width, box->rect.height, arena);

            /* Margin collapsing: collapse top margin with previous bottom. */
            float collapsed = collapse_margins(prev_margin_bottom, child->margin.top);

            child->rect.x = child->margin.left + child->border.left + child->padding.left;
            child->rect.y = cursor_y + collapsed + child->border.top + child->padding.top;

            cursor_y = child->rect.y + child->rect.height +
                       child->padding.bottom + child->border.bottom;
            prev_margin_bottom = child->margin.bottom;
            apply_relative_offset(child, content_width, box->rect.height);
            break;
        }

        case BOX_GRID: {
            layout_grid(child, content_width, box->rect.height, arena);

            /* Margin collapsing: collapse top margin with previous bottom. */
            float collapsed = collapse_margins(prev_margin_bottom, child->margin.top);

            child->rect.x = child->margin.left + child->border.left + child->padding.left;
            child->rect.y = cursor_y + collapsed + child->border.top + child->padding.top;

            cursor_y = child->rect.y + child->rect.height +
                       child->padding.bottom + child->border.bottom;
            prev_margin_bottom = child->margin.bottom;
            apply_relative_offset(child, content_width, box->rect.height);
            break;
        }

        case BOX_BLOCK:
        case BOX_ANONYMOUS_BLOCK: {
            layout_block(child, content_width, box->rect.height, arena);

            /* Margin collapsing: collapse top margin with previous bottom. */
            float collapsed = collapse_margins(prev_margin_bottom, child->margin.top);

            child->rect.x = child->margin.left + child->border.left + child->padding.left;
            child->rect.y = cursor_y + collapsed + child->border.top + child->padding.top;

            cursor_y = child->rect.y + child->rect.height +
                       child->padding.bottom + child->border.bottom;
            prev_margin_bottom = child->margin.bottom;
            apply_relative_offset(child, content_width, box->rect.height);
            break;
        }

        default:
            /* Table rows, cells, etc. — simplified recursive layout. */
            layout_block(child, content_width, box->rect.height, arena);
            child->rect.x = 0;
            child->rect.y = cursor_y;
            cursor_y += layout_box_outer_height(child);
            prev_margin_bottom = 0;
            apply_relative_offset(child, content_width, box->rect.height);
            break;
        }
    }

    /* Auto height: set to content height, including float extents. */
    if (box->style && box->style->height.type == VAL_AUTO) {
        float content_h = cursor_y + prev_margin_bottom;
        if (float_max_bottom > content_h)
            content_h = float_max_bottom;
        box->rect.height = content_h;
    }
}
