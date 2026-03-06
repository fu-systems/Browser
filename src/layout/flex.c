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

/* ── Position relative helper ──────────────────────────────────────── */

static void flex_apply_relative(LayoutBox *child, float cw, float ch)
{
    if (!child->style || child->style->position != POSITION_RELATIVE) return;
    float fs = child->style->font_size;
    if (child->style->top.type != VAL_AUTO)
        child->rect.y += flex_resolve_len(child->style->top, fs, ch);
    else if (child->style->bottom.type != VAL_AUTO)
        child->rect.y -= flex_resolve_len(child->style->bottom, fs, ch);
    if (child->style->left.type != VAL_AUTO)
        child->rect.x += flex_resolve_len(child->style->left, fs, cw);
    else if (child->style->right.type != VAL_AUTO)
        child->rect.x -= flex_resolve_len(child->style->right, fs, cw);
}

/* ── Inline type check ─────────────────────────────────────────────── */

static bool flex_is_inline(LayoutBoxType t)
{
    return t == BOX_INLINE || t == BOX_TEXT || t == BOX_INLINE_BLOCK;
}

/* Check if a child should be skipped from flex flow. */
static bool flex_skip_child(const LayoutBox *c)
{
    if (!c->style) return false;
    if (c->style->display == DISPLAY_NONE) return true;
    if (c->style->position == POSITION_ABSOLUTE ||
        c->style->position == POSITION_FIXED) return true;
    return false;
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

    /* Handle absolutely/fixed positioned children: lay them out but remove from flex flow. */
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (!c->style) continue;
        if (c->style->display == DISPLAY_NONE) continue;
        if (c->style->position == POSITION_ABSOLUTE ||
            c->style->position == POSITION_FIXED) {
            if (flex_is_inline(c->type))
                layout_inline(c, content_w, arena);
            else
                layout_block(c, content_w, content_h, arena);

            float abs_x = 0, abs_y = 0;
            float cfs = c->style->font_size;
            if (c->style->left.type != VAL_AUTO)
                abs_x = flex_resolve_len(c->style->left, cfs, content_w);
            else if (c->style->right.type != VAL_AUTO) {
                float r_val = flex_resolve_len(c->style->right, cfs, content_w);
                abs_x = content_w - c->rect.width - c->padding.left - c->padding.right
                      - c->border.left - c->border.right - c->margin.left - c->margin.right - r_val;
            }
            if (c->style->top.type != VAL_AUTO)
                abs_y = flex_resolve_len(c->style->top, cfs, content_h);
            else if (c->style->bottom.type != VAL_AUTO) {
                float b_val = flex_resolve_len(c->style->bottom, cfs, content_h);
                abs_y = content_h - c->rect.height - c->padding.top - c->padding.bottom
                      - c->border.top - c->border.bottom - c->margin.top - c->margin.bottom - b_val;
            }
            c->rect.x = abs_x + c->margin.left + c->border.left + c->padding.left;
            c->rect.y = abs_y + c->margin.top + c->border.top + c->padding.top;
        }
    }

    /* Resolve gap values. */
    float main_gap = 0, cross_gap = 0;
    if (s->column_gap.type != VAL_NONE && s->column_gap.type != VAL_AUTO) {
        float g = flex_resolve_len(s->column_gap, fs, is_row ? content_w : content_h);
        if (is_row) main_gap = g; else cross_gap = g;
    }
    if (s->row_gap.type != VAL_NONE && s->row_gap.type != VAL_AUTO) {
        float g = flex_resolve_len(s->row_gap, fs, is_row ? content_h : content_w);
        if (is_row) cross_gap = g; else main_gap = g;
    }

    /* Count visible children (excluding absolute/fixed positioned). */
    int child_count = 0;
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (flex_skip_child(c)) continue;
        child_count++;
    }

    if (child_count == 0) {
        box->rect.height = specified_h >= 0 ? specified_h : 0;
        return;
    }

    /* First pass: layout children at natural size and resolve flex-basis. */
    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (flex_skip_child(c)) continue;

        if (flex_is_inline(c->type)) {
            layout_inline(c, content_w, arena);
        } else {
            layout_block(c, content_w, content_h, arena);
        }

        /* Resolve child's flex-basis. */
        if (c->style && c->style->flex_basis.type != VAL_AUTO) {
            float child_main = flex_resolve_len(c->style->flex_basis, c->style->font_size, main_size);
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
    }

    /* ── Flex line collection ────────────────────────────────────────── */
    /* Collect children into flex lines. For nowrap, all in one line. */

    #define MAX_FLEX_LINES 64
    typedef struct {
        LayoutBox *first;
        int count;
        float total_main;
        float max_cross;
        float total_grow;
        float total_shrink;
    } FlexLine;

    FlexLine lines[MAX_FLEX_LINES];
    int line_count = 0;
    memset(lines, 0, sizeof(lines));

    bool wrap = (s->flex_wrap == FLEXWRAP_WRAP || s->flex_wrap == FLEXWRAP_WRAP_REVERSE);

    if (!wrap) {
        /* Single line — all items. */
        lines[0].first = NULL;
        lines[0].count = 0;
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (flex_skip_child(c)) continue;
            if (!lines[0].first) lines[0].first = c;
            lines[0].count++;

            float child_outer_main;
            if (is_row) {
                child_outer_main = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            } else {
                child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            }
            lines[0].total_main += child_outer_main;

            float child_outer_cross;
            if (is_row) {
                child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            } else {
                child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            }
            if (child_outer_cross > lines[0].max_cross) lines[0].max_cross = child_outer_cross;

            float grow = c->style ? c->style->flex_grow : 0;
            float shrink = c->style ? c->style->flex_shrink : 1;
            lines[0].total_grow += grow;
            lines[0].total_shrink += shrink;
        }
        line_count = 1;
    } else {
        /* Wrap: split into lines based on main_size. */
        float line_main = 0;
        int items_in_line = 0;

        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (flex_skip_child(c)) continue;

            float child_outer_main;
            if (is_row) {
                child_outer_main = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            } else {
                child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            }

            float gap_add = items_in_line > 0 ? main_gap : 0;

            /* Need a new line? */
            if (items_in_line > 0 &&
                line_main + gap_add + child_outer_main > main_size &&
                line_count < MAX_FLEX_LINES) {
                line_count++;
                line_main = 0;
                items_in_line = 0;
            }

            if (line_count == 0) line_count = 1;
            FlexLine *fl = &lines[line_count - 1];
            if (!fl->first) fl->first = c;
            fl->count++;
            fl->total_main += child_outer_main + (items_in_line > 0 ? main_gap : 0);
            items_in_line++;

            float child_outer_cross;
            if (is_row) {
                child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            } else {
                child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            }
            if (child_outer_cross > fl->max_cross) fl->max_cross = child_outer_cross;

            fl->total_grow += c->style ? c->style->flex_grow : 0;
            fl->total_shrink += c->style ? c->style->flex_shrink : 1;

            line_main = fl->total_main;
        }
        if (line_count == 0) line_count = 1;
    }

    /* For column flex with auto height, use total of all lines. */
    if (!is_row && specified_h < 0) {
        float total = 0;
        for (int li = 0; li < line_count; li++)
            total += lines[li].total_main;
        main_size = total;
    }

    /* ── Grow/shrink per line ────────────────────────────────────────── */

    for (int li = 0; li < line_count; li++) {
        FlexLine *fl = &lines[li];
        float free_space = main_size - fl->total_main;

        if (free_space > 0 && fl->total_grow > 0) {
            int idx = 0;
            for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
                if (flex_skip_child(c)) continue;
                /* Find items belonging to this line. */
                if (c == fl->first || (idx > 0 && idx < fl->count)) {
                    if (c == fl->first) idx = 1; else idx++;
                    float grow = c->style ? c->style->flex_grow : 0;
                    if (grow > 0) {
                        float extra = free_space * (grow / fl->total_grow);
                        if (is_row) c->rect.width += extra;
                        else c->rect.height += extra;
                    }
                    if (idx >= fl->count) break;
                    continue;
                }
                if (idx >= fl->count) break;
            }
            /* Relayout grown children. */
            idx = 0;
            for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
                if (flex_skip_child(c)) continue;
                if (c == fl->first) idx = 1; else if (idx > 0) idx++;
                if (idx == 0) continue;
                float grow = c->style ? c->style->flex_grow : 0;
                if (grow > 0) {
                    if (flex_is_inline(c->type)) {
                        layout_inline(c, is_row ? c->rect.width : content_w, arena);
                    } else if (is_row) {
                        layout_block(c, c->rect.width, content_h, arena);
                    } else {
                        float grown_h = c->rect.height;
                        layout_block(c, content_w, grown_h, arena);
                        c->rect.height = grown_h;
                    }
                }
                if (idx >= fl->count) break;
            }
        } else if (free_space < 0 && fl->total_shrink > 0) {
            float deficit = -free_space;
            int idx = 0;
            for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
                if (flex_skip_child(c)) continue;
                if (c == fl->first) idx = 1; else if (idx > 0) idx++;
                if (idx == 0) continue;
                float shrink = c->style ? c->style->flex_shrink : 1;
                if (shrink > 0) {
                    float reduction = deficit * (shrink / fl->total_shrink);
                    if (is_row) {
                        c->rect.width -= reduction;
                        if (c->rect.width < 0) c->rect.width = 0;
                    } else {
                        c->rect.height -= reduction;
                        if (c->rect.height < 0) c->rect.height = 0;
                    }
                }
                if (idx >= fl->count) break;
            }
        }

        /* Recompute line's max_cross after grow/shrink. */
        fl->max_cross = 0;
        int idx = 0;
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (flex_skip_child(c)) continue;
            if (c == fl->first) idx = 1; else if (idx > 0) idx++;
            if (idx == 0) continue;
            float child_outer_cross;
            if (is_row) {
                child_outer_cross = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            } else {
                child_outer_cross = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            }
            if (child_outer_cross > fl->max_cross) fl->max_cross = child_outer_cross;
            if (idx >= fl->count) break;
        }
    }

    /* ── Positioning ─────────────────────────────────────────────────── */

    /* Compute total cross size from all lines. */
    float total_cross = 0;
    for (int li = 0; li < line_count; li++) {
        total_cross += lines[li].max_cross;
        if (li > 0) total_cross += cross_gap;
    }

    /* If height is auto, use total_cross (row) or sum of line mains (column). */
    float actual_cross;
    if (is_row) {
        actual_cross = (specified_h >= 0) ? specified_h : total_cross;
    } else {
        actual_cross = (s->width.type != VAL_AUTO) ? content_w : total_cross;
    }

    float cross_offset = 0;
    bool wrap_reverse = (s->flex_wrap == FLEXWRAP_WRAP_REVERSE);

    for (int li = 0; li < line_count; li++) {
        int line_idx = wrap_reverse ? (line_count - 1 - li) : li;
        FlexLine *fl = &lines[line_idx];
        float line_cross = fl->max_cross;

        /* Compute free space on main axis for this line's items. */
        float line_total_main = 0;
        int line_items = 0;
        int idx = 0;
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (flex_skip_child(c)) continue;
            if (c == fl->first) idx = 1; else if (idx > 0) idx++;
            if (idx == 0) continue;
            float child_outer_main;
            if (is_row) {
                child_outer_main = c->rect.width + c->padding.left + c->padding.right
                    + c->border.left + c->border.right + c->margin.left + c->margin.right;
            } else {
                child_outer_main = c->rect.height + c->padding.top + c->padding.bottom
                    + c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;
            }
            line_total_main += child_outer_main;
            line_items++;
            if (idx >= fl->count) break;
        }

        /* Add gaps to line_total_main. */
        if (line_items > 1) line_total_main += main_gap * (line_items - 1);

        float free_space = main_size - line_total_main;
        if (free_space < 0) free_space = 0;

        float main_pos = 0;
        float item_gap = main_gap;

        switch (jc) {
        case JC_FLEX_START: main_pos = 0; break;
        case JC_FLEX_END:   main_pos = free_space; break;
        case JC_CENTER:     main_pos = free_space / 2.0f; break;
        case JC_SPACE_BETWEEN:
            main_pos = 0;
            item_gap = line_items > 1 ? free_space / (float)(line_items - 1) + main_gap : main_gap;
            break;
        case JC_SPACE_AROUND:
            item_gap = line_items > 0 ? free_space / (float)line_items + main_gap : main_gap;
            main_pos = (item_gap - main_gap) / 2.0f;
            break;
        case JC_SPACE_EVENLY:
            item_gap = line_items > 0 ? free_space / (float)(line_items + 1) + main_gap : main_gap;
            main_pos = item_gap - main_gap;
            break;
        }

        if (is_reverse) {
            main_pos = main_size - main_pos;
        }

        /* Position items in this line. */
        int item_idx = 0;
        idx = 0;
        for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
            if (flex_skip_child(c)) continue;
            if (c == fl->first) idx = 1; else if (idx > 0) idx++;
            if (idx == 0) continue;

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
                cross_pos = line_cross - child_outer_cross;
                if (cross_pos < 0) cross_pos = 0;
                break;
            case AI_CENTER:
                cross_pos = (line_cross - child_outer_cross) / 2.0f;
                if (cross_pos < 0) cross_pos = 0;
                break;
            case AI_STRETCH:
                if (is_row && c->style && c->style->height.type == VAL_AUTO) {
                    float stretch_h = line_cross - c->padding.top - c->padding.bottom
                        - c->border.top - c->border.bottom
                        - c->margin.top - c->margin.bottom;
                    if (stretch_h > c->rect.height) c->rect.height = stretch_h;
                } else if (!is_row && c->style && c->style->width.type == VAL_AUTO) {
                    float stretch_w = line_cross - c->padding.left - c->padding.right
                        - c->border.left - c->border.right
                        - c->margin.left - c->margin.right;
                    if (stretch_w > c->rect.width) c->rect.width = stretch_w;
                }
                cross_pos = 0;
                break;
            }

            float main_start;
            if (is_reverse) {
                main_start = main_pos - child_outer_main;
            } else {
                main_start = main_pos;
            }

            if (is_row) {
                c->rect.x = main_start + c->margin.left + c->border.left + c->padding.left;
                c->rect.y = cross_offset + cross_pos + c->margin.top + c->border.top + c->padding.top;
            } else {
                c->rect.x = cross_offset + cross_pos + c->margin.left + c->border.left + c->padding.left;
                c->rect.y = main_start + c->margin.top + c->border.top + c->padding.top;
            }

            flex_apply_relative(c, content_w, content_h);

            if (is_reverse) {
                main_pos -= child_outer_main + (item_idx > 0 ? 0 : 0) + item_gap;
            } else {
                main_pos += child_outer_main + item_gap;
            }
            item_idx++;
            if (idx >= fl->count) break;
        }

        cross_offset += line_cross + cross_gap;
    }

    /* Set container height. */
    if (specified_h >= 0) {
        box->rect.height = specified_h;
    } else if (is_row) {
        box->rect.height = total_cross;
    } else {
        float total_main = 0;
        for (int li = 0; li < line_count; li++)
            total_main += lines[li].total_main;
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
