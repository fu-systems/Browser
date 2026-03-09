/*
 * Pane — Grid Layout Implementation
 *
 * CSS Grid algorithm supporting:
 *   - grid-template-columns / grid-template-rows (fixed, fr, auto, %)
 *   - grid-auto-columns / grid-auto-rows
 *   - grid-auto-flow: row, column, dense variants
 *   - grid-column-start/end, grid-row-start/end (line numbers)
 *   - row-gap / column-gap
 *   - justify-items / align-items / align-content
 *   - min/max width/height constraints
 */

#include "grid.h"
#include "block.h"
#include "flex.h"
#include "inline.h"
#include <string.h>
#include <math.h>

/* ── Constants ─────────────────────────────────────────────────────── */

#define GRID_MAX_TRACKS 64
#define GRID_MAX_ITEMS  256

/* ── Resolve length to pixels ──────────────────────────────────────── */

static float grid_resolve_len(CssValue val, float font_size, float containing)
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

/* ── Position relative helper ──────────────────────────────────────── */

static void grid_apply_relative(LayoutBox *child, float cw, float ch)
{
    if (!child->style || child->style->position != POSITION_RELATIVE) return;
    float fs = child->style->font_size;
    if (child->style->top.type != VAL_AUTO)
        child->rect.y += grid_resolve_len(child->style->top, fs, ch);
    else if (child->style->bottom.type != VAL_AUTO)
        child->rect.y -= grid_resolve_len(child->style->bottom, fs, ch);
    if (child->style->left.type != VAL_AUTO)
        child->rect.x += grid_resolve_len(child->style->left, fs, cw);
    else if (child->style->right.type != VAL_AUTO)
        child->rect.x -= grid_resolve_len(child->style->right, fs, cw);
}

/* ── Track sizing ──────────────────────────────────────────────────── */

typedef struct {
    float base;     /* resolved base size in px (0 for fr tracks before distribution) */
    float fr;       /* fr fraction (0 if not a fr track) */
    bool  is_auto;  /* auto-sized track */
} GridTrack;

/* Parse a track list from a CssValue into an array of GridTrack.
 * Returns the number of explicit tracks parsed. */
static int parse_track_list(const CssValue *val, GridTrack *tracks, int max,
                            float font_size, float containing)
{
    if (!val || val->type == VAL_NONE || val->type == VAL_AUTO)
        return 0;

    /* Single value (not a list). */
    if (val->type != VAL_LIST) {
        if (val->type == VAL_LENGTH || val->type == VAL_PERCENTAGE ||
            val->type == VAL_NUMBER) {
            if (val->type == VAL_LENGTH && val->length.unit == UNIT_FR) {
                tracks[0].fr = val->length.magnitude;
                tracks[0].base = 0;
                tracks[0].is_auto = false;
            } else {
                tracks[0].base = grid_resolve_len(*val, font_size, containing);
                tracks[0].fr = 0;
                tracks[0].is_auto = false;
            }
            return 1;
        }
        if (val->type == VAL_KEYWORD && val->string &&
            strcmp(val->string, "auto") == 0) {
            tracks[0].is_auto = true;
            tracks[0].base = 0;
            tracks[0].fr = 0;
            return 1;
        }
        return 0;
    }

    /* List of values. */
    int count = val->list.count;
    if (count > max) count = max;

    for (int i = 0; i < count; i++) {
        CssValue *item = &val->list.items[i];
        tracks[i].fr = 0;
        tracks[i].base = 0;
        tracks[i].is_auto = false;

        if (item->type == VAL_LENGTH && item->length.unit == UNIT_FR) {
            tracks[i].fr = item->length.magnitude;
        } else if (item->type == VAL_AUTO ||
                   (item->type == VAL_KEYWORD && item->string &&
                    strcmp(item->string, "auto") == 0)) {
            tracks[i].is_auto = true;
        } else {
            tracks[i].base = grid_resolve_len(*item, font_size, containing);
        }
    }
    return count;
}

/* Resolve a single auto-track size value. */
static float resolve_auto_track(const CssValue *val, float font_size,
                                float containing, float fallback)
{
    if (!val || val->type == VAL_AUTO || val->type == VAL_NONE)
        return fallback;
    if (val->type == VAL_LENGTH || val->type == VAL_PERCENTAGE ||
        val->type == VAL_NUMBER)
        return grid_resolve_len(*val, font_size, containing);
    return fallback;
}

/* ── Grid item placement ───────────────────────────────────────────── */

typedef struct {
    LayoutBox *box;
    int col_start, col_end;   /* 0-based column indices */
    int row_start, row_end;   /* 0-based row indices */
} GridItem;

/* Parse a grid line value to a 0-based index. Returns -1 for auto. */
static int parse_line(const CssValue *val)
{
    if (!val) return -1;
    if (val->type == VAL_AUTO || val->type == VAL_NONE) return -1;
    if (val->type == VAL_NUMBER) {
        int n = (int)val->number;
        /* CSS grid lines are 1-based; convert to 0-based track index.
         * Line 1 = before track 0, line 2 = before track 1, etc.
         * Negative lines count from the end (handled later). */
        return n > 0 ? n - 1 : n;
    }
    if (val->type == VAL_LENGTH)
        return (int)val->length.magnitude - 1;
    return -1;
}

/* ── Alignment helpers ─────────────────────────────────────────────── */

typedef enum {
    GRID_ALIGN_START,
    GRID_ALIGN_END,
    GRID_ALIGN_CENTER,
    GRID_ALIGN_STRETCH,
} GridAlign;

static GridAlign parse_grid_align(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string)
        return GRID_ALIGN_STRETCH;
    const char *s = val->string;
    if (strcmp(s, "start") == 0 || strcmp(s, "flex-start") == 0) return GRID_ALIGN_START;
    if (strcmp(s, "end") == 0 || strcmp(s, "flex-end") == 0)     return GRID_ALIGN_END;
    if (strcmp(s, "center") == 0)                                 return GRID_ALIGN_CENTER;
    return GRID_ALIGN_STRETCH;
}

/* ── Inline type check ─────────────────────────────────────────────── */

static bool grid_is_inline(LayoutBoxType t)
{
    return t == BOX_INLINE || t == BOX_TEXT || t == BOX_INLINE_BLOCK;
}

/* Check if a child should be skipped from grid flow. */
static bool grid_skip_child(const LayoutBox *c)
{
    if (!c->style) return false;
    if (c->style->display == DISPLAY_NONE) return true;
    if (c->style->position == POSITION_ABSOLUTE ||
        c->style->position == POSITION_FIXED) return true;
    return false;
}

/* ── Grid Layout ───────────────────────────────────────────────────── */

void layout_grid(LayoutBox *box, float containing_width, float containing_height,
                 Arena *arena)
{
    ComputedStyle *s = box->style;
    if (!s) return;

    float fs = s->font_size;

    /* ── Resolve box model edges ──────────────────────────────────── */

    box->margin = s->margin;
    box->padding = s->padding;
    box->border = s->border_width;

    /* Resolve container width. */
    if (s->width.type != VAL_AUTO) {
        float w = grid_resolve_len(s->width, fs, containing_width);
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

    /* Auto margin centering. */
    if (s->width.type != VAL_AUTO) {
        float remaining = containing_width - box->rect.width
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

    /* Resolve container height. */
    float specified_h = -1;
    if (s->height.type != VAL_AUTO) {
        specified_h = grid_resolve_len(s->height, fs, containing_height);
        if (s->box_sizing == BOX_BORDER_BOX) {
            specified_h -= box->padding.top + box->padding.bottom +
                           box->border.top + box->border.bottom;
            if (specified_h < 0) specified_h = 0;
        }
    }

    float content_w = box->rect.width;

    /* ── Resolve gaps ─────────────────────────────────────────────── */

    float col_gap = 0, row_gap = 0;
    if (s->column_gap.type != VAL_NONE && s->column_gap.type != VAL_AUTO)
        col_gap = grid_resolve_len(s->column_gap, fs, content_w);
    if (s->row_gap.type != VAL_NONE && s->row_gap.type != VAL_AUTO)
        row_gap = grid_resolve_len(s->row_gap, fs, content_w);

    /* ── Handle absolutely/fixed positioned children ─────────────── */

    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (!c->style) continue;
        if (c->style->display == DISPLAY_NONE) continue;
        if (c->style->position == POSITION_ABSOLUTE ||
            c->style->position == POSITION_FIXED) {
            if (grid_is_inline(c->type))
                layout_inline(c, content_w, arena);
            else
                layout_block(c, content_w, specified_h >= 0 ? specified_h : containing_height, arena);

            float abs_x = 0, abs_y = 0;
            float cfs = c->style->font_size;
            if (c->style->left.type != VAL_AUTO)
                abs_x = grid_resolve_len(c->style->left, cfs, content_w);
            else if (c->style->right.type != VAL_AUTO) {
                float r_val = grid_resolve_len(c->style->right, cfs, content_w);
                abs_x = content_w - c->rect.width - c->padding.left - c->padding.right
                      - c->border.left - c->border.right - c->margin.left - c->margin.right - r_val;
            }
            float ch = specified_h >= 0 ? specified_h : containing_height;
            if (c->style->top.type != VAL_AUTO)
                abs_y = grid_resolve_len(c->style->top, cfs, ch);
            else if (c->style->bottom.type != VAL_AUTO) {
                float b_val = grid_resolve_len(c->style->bottom, cfs, ch);
                abs_y = ch - c->rect.height - c->padding.top - c->padding.bottom
                      - c->border.top - c->border.bottom - c->margin.top - c->margin.bottom - b_val;
            }
            c->rect.x = abs_x + c->margin.left + c->border.left + c->padding.left;
            c->rect.y = abs_y + c->margin.top + c->border.top + c->padding.top;
        }
    }

    /* ── Collect grid items ───────────────────────────────────────── */

    GridItem items[GRID_MAX_ITEMS];
    int item_count = 0;

    for (LayoutBox *c = box->first_child; c; c = c->next_sibling) {
        if (grid_skip_child(c)) continue;
        if (item_count >= GRID_MAX_ITEMS) break;
        items[item_count].box = c;
        items[item_count].col_start = -1;
        items[item_count].col_end = -1;
        items[item_count].row_start = -1;
        items[item_count].row_end = -1;

        if (c->style) {
            items[item_count].col_start = parse_line(&c->style->grid_column_start);
            items[item_count].col_end = parse_line(&c->style->grid_column_end);
            items[item_count].row_start = parse_line(&c->style->grid_row_start);
            items[item_count].row_end = parse_line(&c->style->grid_row_end);
        }
        item_count++;
    }

    if (item_count == 0) {
        box->rect.height = specified_h >= 0 ? specified_h : 0;
        return;
    }

    /* ── Parse explicit tracks ────────────────────────────────────── */

    GridTrack col_tracks[GRID_MAX_TRACKS];
    GridTrack row_tracks[GRID_MAX_TRACKS];
    int num_cols = parse_track_list(&s->grid_template_columns, col_tracks,
                                    GRID_MAX_TRACKS, fs, content_w);
    int num_rows = parse_track_list(&s->grid_template_rows, row_tracks,
                                    GRID_MAX_TRACKS, fs,
                                    specified_h >= 0 ? specified_h : containing_height);

    /* Determine grid dimensions from explicit tracks and item placement. */
    bool flow_column = (s->grid_auto_flow == GRID_FLOW_COLUMN ||
                        s->grid_auto_flow == GRID_FLOW_COLUMN_DENSE);

    /* Find the maximum column/row referenced by items. */
    int max_col = num_cols;
    int max_row = num_rows;
    for (int i = 0; i < item_count; i++) {
        if (items[i].col_end > max_col) max_col = items[i].col_end;
        if (items[i].col_start >= 0 && items[i].col_start + 1 > max_col)
            max_col = items[i].col_start + 1;
        if (items[i].row_end > max_row) max_row = items[i].row_end;
        if (items[i].row_start >= 0 && items[i].row_start + 1 > max_row)
            max_row = items[i].row_start + 1;
    }

    /* If no explicit columns, create enough for auto-placement. */
    if (num_cols == 0 && !flow_column) {
        /* Default: single column filling width, or guess from item count. */
        num_cols = max_col > 0 ? max_col : 1;
    }
    if (num_cols == 0) num_cols = 1;

    /* Ensure enough rows for all items. */
    int needed_rows = max_row;
    if (!flow_column) {
        /* Row flow: need at least ceil(item_count / num_cols) rows. */
        int auto_items = 0;
        for (int i = 0; i < item_count; i++) {
            if (items[i].row_start < 0) auto_items++;
        }
        int min_rows = (auto_items + num_cols - 1) / num_cols;
        if (min_rows > needed_rows) needed_rows = min_rows;
    } else {
        /* Column flow: need at least ceil(item_count / max_rows_or_1). */
        if (num_rows == 0) num_rows = 1;
        int auto_items = 0;
        for (int i = 0; i < item_count; i++) {
            if (items[i].col_start < 0) auto_items++;
        }
        int min_cols = (auto_items + num_rows - 1) / num_rows;
        if (min_cols > max_col) max_col = min_cols;
        if (max_col > num_cols) num_cols = max_col;
        needed_rows = num_rows;
    }
    if (needed_rows < 1) needed_rows = 1;
    if (needed_rows > GRID_MAX_TRACKS) needed_rows = GRID_MAX_TRACKS;
    if (num_cols > GRID_MAX_TRACKS) num_cols = GRID_MAX_TRACKS;

    /* Fill in implicit tracks beyond explicit ones. */
    float auto_col_size = resolve_auto_track(&s->grid_auto_columns, fs, content_w, 0);
    float auto_row_size = resolve_auto_track(&s->grid_auto_rows, fs,
                                              specified_h >= 0 ? specified_h : containing_height, 0);
    for (int i = num_cols; i < max_col && i < GRID_MAX_TRACKS; i++) {
        col_tracks[i].base = auto_col_size;
        col_tracks[i].fr = 0;
        col_tracks[i].is_auto = (auto_col_size == 0);
    }
    if (max_col > num_cols) num_cols = max_col;

    int total_rows = needed_rows > (int)num_rows ? needed_rows : (int)num_rows;
    if (total_rows > GRID_MAX_TRACKS) total_rows = GRID_MAX_TRACKS;
    for (int i = num_rows; i < total_rows; i++) {
        row_tracks[i].base = auto_row_size;
        row_tracks[i].fr = 0;
        row_tracks[i].is_auto = (auto_row_size == 0);
    }
    num_rows = total_rows;

    /* ── Auto-place items without explicit positions ──────────────── */

    /* Track cell occupancy: occupied[row * num_cols + col] */
    bool occupied[GRID_MAX_TRACKS * GRID_MAX_TRACKS];
    memset(occupied, 0, sizeof(occupied));

    /* First pass: place items with explicit positions. */
    for (int i = 0; i < item_count; i++) {
        if (items[i].col_start >= 0 && items[i].row_start >= 0) {
            if (items[i].col_end < 0) items[i].col_end = items[i].col_start + 1;
            if (items[i].row_end < 0) items[i].row_end = items[i].row_start + 1;

            /* Clamp to grid bounds. */
            if (items[i].col_end > num_cols) items[i].col_end = num_cols;
            if (items[i].row_end > num_rows) items[i].row_end = num_rows;

            for (int r = items[i].row_start; r < items[i].row_end && r < num_rows; r++) {
                for (int c = items[i].col_start; c < items[i].col_end && c < num_cols; c++) {
                    occupied[r * num_cols + c] = true;
                }
            }
        }
    }

    /* Second pass: auto-place remaining items. */
    int auto_row = 0, auto_col = 0;
    for (int i = 0; i < item_count; i++) {
        if (items[i].col_start >= 0 && items[i].row_start >= 0) continue;

        /* Item has partial placement — resolve what we can. */
        int span_c = 1, span_r = 1;
        if (items[i].col_start >= 0 && items[i].col_end > items[i].col_start)
            span_c = items[i].col_end - items[i].col_start;
        if (items[i].row_start >= 0 && items[i].row_end > items[i].row_start)
            span_r = items[i].row_end - items[i].row_start;

        if (items[i].col_start >= 0) {
            /* Column fixed, find next available row. */
            int c = items[i].col_start;
            bool placed = false;
            for (int r = 0; r < num_rows && !placed; r++) {
                bool fits = true;
                for (int dr = 0; dr < span_r && fits; dr++) {
                    for (int dc = 0; dc < span_c && fits; dc++) {
                        if (r + dr >= num_rows || c + dc >= num_cols ||
                            occupied[(r + dr) * num_cols + (c + dc)])
                            fits = false;
                    }
                }
                if (fits) {
                    items[i].row_start = r;
                    items[i].row_end = r + span_r;
                    items[i].col_end = c + span_c;
                    placed = true;
                }
            }
            if (!placed) {
                items[i].row_start = num_rows - 1;
                items[i].row_end = num_rows;
                items[i].col_end = c + span_c;
            }
        } else if (items[i].row_start >= 0) {
            /* Row fixed, find next available column. */
            int r = items[i].row_start;
            bool placed = false;
            for (int c = 0; c < num_cols && !placed; c++) {
                bool fits = true;
                for (int dr = 0; dr < span_r && fits; dr++) {
                    for (int dc = 0; dc < span_c && fits; dc++) {
                        if (r + dr >= num_rows || c + dc >= num_cols ||
                            occupied[(r + dr) * num_cols + (c + dc)])
                            fits = false;
                    }
                }
                if (fits) {
                    items[i].col_start = c;
                    items[i].col_end = c + span_c;
                    items[i].row_end = r + span_r;
                    placed = true;
                }
            }
            if (!placed) {
                items[i].col_start = num_cols - 1;
                items[i].col_end = num_cols;
                items[i].row_end = r + span_r;
            }
        } else {
            /* Fully auto-placed. */
            bool placed = false;
            if (flow_column) {
                /* Column flow: advance down rows, then across columns. */
                while (auto_col < num_cols && !placed) {
                    while (auto_row < num_rows && !placed) {
                        if (!occupied[auto_row * num_cols + auto_col]) {
                            items[i].row_start = auto_row;
                            items[i].row_end = auto_row + 1;
                            items[i].col_start = auto_col;
                            items[i].col_end = auto_col + 1;
                            auto_row++;
                            placed = true;
                        } else {
                            auto_row++;
                        }
                    }
                    if (!placed) {
                        auto_row = 0;
                        auto_col++;
                    }
                }
            } else {
                /* Row flow: advance across columns, then down rows. */
                while (auto_row < num_rows && !placed) {
                    while (auto_col < num_cols && !placed) {
                        if (!occupied[auto_row * num_cols + auto_col]) {
                            items[i].col_start = auto_col;
                            items[i].col_end = auto_col + 1;
                            items[i].row_start = auto_row;
                            items[i].row_end = auto_row + 1;
                            auto_col++;
                            placed = true;
                        } else {
                            auto_col++;
                        }
                    }
                    if (!placed) {
                        auto_col = 0;
                        auto_row++;
                    }
                }
            }
            if (!placed) {
                /* Overflow: place in last cell. */
                items[i].col_start = num_cols - 1;
                items[i].col_end = num_cols;
                items[i].row_start = num_rows - 1;
                items[i].row_end = num_rows;
            }
        }

        /* Mark cells as occupied. */
        for (int r = items[i].row_start; r < items[i].row_end && r < num_rows; r++) {
            for (int c = items[i].col_start; c < items[i].col_end && c < num_cols; c++) {
                occupied[r * num_cols + c] = true;
            }
        }
    }

    /* ── Layout each item to determine intrinsic sizes ────────────── */

    for (int i = 0; i < item_count; i++) {
        LayoutBox *c = items[i].box;
        if (grid_is_inline(c->type)) {
            layout_inline(c, content_w, arena);
        } else if (c->type == BOX_FLEX) {
            layout_flex(c, content_w, containing_height, arena);
        } else {
            layout_block(c, content_w, containing_height, arena);
        }
    }

    /* ── Resolve column track sizes ───────────────────────────────── */

    float col_sizes[GRID_MAX_TRACKS];
    float total_fixed_cols = 0;
    float total_col_fr = 0;
    float total_col_gaps = (num_cols > 1) ? col_gap * (float)(num_cols - 1) : 0;

    /* For auto tracks, determine max content width of items in that column. */
    for (int c = 0; c < num_cols; c++) {
        col_sizes[c] = col_tracks[c].base;
        if (col_tracks[c].fr > 0) {
            total_col_fr += col_tracks[c].fr;
        } else if (col_tracks[c].is_auto || col_tracks[c].base == 0) {
            /* Auto: find max width among items in this column. */
            float max_w = 0;
            for (int i = 0; i < item_count; i++) {
                if (items[i].col_start <= c && items[i].col_end > c &&
                    items[i].col_end - items[i].col_start == 1) {
                    float item_w = items[i].box->rect.width +
                        items[i].box->padding.left + items[i].box->padding.right +
                        items[i].box->border.left + items[i].box->border.right +
                        items[i].box->margin.left + items[i].box->margin.right;
                    if (item_w > max_w) max_w = item_w;
                }
            }
            col_sizes[c] = max_w;
            total_fixed_cols += max_w;
        } else {
            total_fixed_cols += col_tracks[c].base;
        }
    }

    /* Distribute fr space. */
    if (total_col_fr > 0) {
        float available = content_w - total_fixed_cols - total_col_gaps;
        if (available < 0) available = 0;
        for (int c = 0; c < num_cols; c++) {
            if (col_tracks[c].fr > 0) {
                col_sizes[c] = available * (col_tracks[c].fr / total_col_fr);
            }
        }
    }

    /* If all columns are auto and no fr, distribute equally. */
    if (total_col_fr == 0 && total_fixed_cols == 0 && num_cols > 0) {
        float available = content_w - total_col_gaps;
        if (available < 0) available = 0;
        float each = available / (float)num_cols;
        for (int c = 0; c < num_cols; c++) {
            col_sizes[c] = each;
        }
    }

    /* ── Re-layout items with resolved column widths ──────────────── */

    for (int i = 0; i < item_count; i++) {
        int cs = items[i].col_start;
        int ce = items[i].col_end;
        float item_avail_w = 0;
        for (int c = cs; c < ce && c < num_cols; c++) {
            item_avail_w += col_sizes[c];
            if (c > cs) item_avail_w += col_gap;
        }

        LayoutBox *c = items[i].box;
        if (grid_is_inline(c->type)) {
            layout_inline(c, item_avail_w, arena);
        } else if (c->type == BOX_FLEX) {
            layout_flex(c, item_avail_w, containing_height, arena);
        } else {
            layout_block(c, item_avail_w, containing_height, arena);
        }
    }

    /* ── Resolve row track sizes ──────────────────────────────────── */

    float row_sizes[GRID_MAX_TRACKS];
    float total_fixed_rows = 0;
    float total_row_fr = 0;

    for (int r = 0; r < num_rows; r++) {
        row_sizes[r] = row_tracks[r].base;
        if (row_tracks[r].fr > 0) {
            total_row_fr += row_tracks[r].fr;
        } else if (row_tracks[r].is_auto || row_tracks[r].base == 0) {
            /* Auto: find max height among items in this row. */
            float max_h = 0;
            for (int i = 0; i < item_count; i++) {
                if (items[i].row_start <= r && items[i].row_end > r &&
                    items[i].row_end - items[i].row_start == 1) {
                    float item_h = items[i].box->rect.height +
                        items[i].box->padding.top + items[i].box->padding.bottom +
                        items[i].box->border.top + items[i].box->border.bottom +
                        items[i].box->margin.top + items[i].box->margin.bottom;
                    if (item_h > max_h) max_h = item_h;
                }
            }
            row_sizes[r] = max_h;
            total_fixed_rows += max_h;
        } else {
            total_fixed_rows += row_tracks[r].base;
        }
    }

    /* Distribute fr space for rows if container has definite height. */
    if (total_row_fr > 0 && specified_h >= 0) {
        float total_row_gaps = (num_rows > 1) ? row_gap * (float)(num_rows - 1) : 0;
        float available = specified_h - total_fixed_rows - total_row_gaps;
        if (available < 0) available = 0;
        for (int r = 0; r < num_rows; r++) {
            if (row_tracks[r].fr > 0) {
                row_sizes[r] = available * (row_tracks[r].fr / total_row_fr);
            }
        }
    }

    /* ── Compute track positions ──────────────────────────────────── */

    float col_pos[GRID_MAX_TRACKS];
    float row_pos[GRID_MAX_TRACKS];

    col_pos[0] = 0;
    for (int c = 1; c < num_cols; c++) {
        col_pos[c] = col_pos[c - 1] + col_sizes[c - 1] + col_gap;
    }

    row_pos[0] = 0;
    for (int r = 1; r < num_rows; r++) {
        row_pos[r] = row_pos[r - 1] + row_sizes[r - 1] + row_gap;
    }

    /* ── Compute container height early for relative offset resolution ── */

    float container_h;
    if (specified_h >= 0) {
        container_h = specified_h;
    } else {
        container_h = 0;
        for (int r = 0; r < num_rows; r++) {
            container_h += row_sizes[r];
        }
        container_h += (num_rows > 1) ? row_gap * (float)(num_rows - 1) : 0;
    }

    /* ── Position items ───────────────────────────────────────────── */

    GridAlign ji = parse_grid_align(&s->justify_items);
    GridAlign ai = parse_grid_align(&s->align_items);

    for (int i = 0; i < item_count; i++) {
        LayoutBox *c = items[i].box;
        int cs = items[i].col_start;
        int rs = items[i].row_start;
        int ce = items[i].col_end;
        int re = items[i].row_end;

        if (cs < 0) cs = 0;
        if (rs < 0) rs = 0;
        if (ce > num_cols) ce = num_cols;
        if (re > num_rows) re = num_rows;

        /* Cell area. */
        float cell_x = col_pos[cs];
        float cell_y = row_pos[rs];
        float cell_w = 0, cell_h = 0;
        for (int cc = cs; cc < ce; cc++) {
            cell_w += col_sizes[cc];
            if (cc > cs) cell_w += col_gap;
        }
        for (int rr = rs; rr < re; rr++) {
            cell_h += row_sizes[rr];
            if (rr > rs) cell_h += row_gap;
        }

        float item_outer_w = c->rect.width + c->padding.left + c->padding.right +
            c->border.left + c->border.right + c->margin.left + c->margin.right;
        float item_outer_h = c->rect.height + c->padding.top + c->padding.bottom +
            c->border.top + c->border.bottom + c->margin.top + c->margin.bottom;

        /* Check align-self override. */
        GridAlign item_ji = ji;
        GridAlign item_ai = ai;
        if (c->style && c->style->align_self.type == VAL_KEYWORD &&
            c->style->align_self.string &&
            strcmp(c->style->align_self.string, "auto") != 0) {
            item_ai = parse_grid_align(&c->style->align_self);
        }

        /* Horizontal (justify-items). */
        float x_offset = 0;
        switch (item_ji) {
        case GRID_ALIGN_START:
            x_offset = 0;
            break;
        case GRID_ALIGN_END:
            x_offset = cell_w - item_outer_w;
            if (x_offset < 0) x_offset = 0;
            break;
        case GRID_ALIGN_CENTER:
            x_offset = (cell_w - item_outer_w) / 2.0f;
            if (x_offset < 0) x_offset = 0;
            break;
        case GRID_ALIGN_STRETCH:
            /* Stretch: expand item to fill cell width. */
            if (c->style && c->style->width.type == VAL_AUTO) {
                float stretch_w = cell_w - c->padding.left - c->padding.right -
                    c->border.left - c->border.right - c->margin.left - c->margin.right;
                if (stretch_w > 0) c->rect.width = stretch_w;
            }
            x_offset = 0;
            break;
        }

        /* Vertical (align-items). */
        float y_offset = 0;
        switch (item_ai) {
        case GRID_ALIGN_START:
            y_offset = 0;
            break;
        case GRID_ALIGN_END:
            y_offset = cell_h - item_outer_h;
            if (y_offset < 0) y_offset = 0;
            break;
        case GRID_ALIGN_CENTER:
            y_offset = (cell_h - item_outer_h) / 2.0f;
            if (y_offset < 0) y_offset = 0;
            break;
        case GRID_ALIGN_STRETCH:
            if (c->style && c->style->height.type == VAL_AUTO) {
                float stretch_h = cell_h - c->padding.top - c->padding.bottom -
                    c->border.top - c->border.bottom - c->margin.top - c->margin.bottom;
                if (stretch_h > c->rect.height) c->rect.height = stretch_h;
            }
            y_offset = 0;
            break;
        }

        c->rect.x = cell_x + x_offset + c->margin.left + c->border.left + c->padding.left;
        c->rect.y = cell_y + y_offset + c->margin.top + c->border.top + c->padding.top;
        grid_apply_relative(c, content_w, container_h);
    }

    /* ── Set container height ─────────────────────────────────────── */

    box->rect.height = container_h;

    /* ── Apply min/max constraints ────────────────────────────────── */

    if (s->min_width.type != VAL_AUTO && s->min_width.type != VAL_NONE) {
        float min_w = grid_resolve_len(s->min_width, fs, containing_width);
        if (box->rect.width < min_w) box->rect.width = min_w;
    }
    if (s->max_width.type != VAL_NONE && s->max_width.type != VAL_AUTO) {
        float max_w = grid_resolve_len(s->max_width, fs, containing_width);
        if (box->rect.width > max_w) box->rect.width = max_w;
    }
    if (s->min_height.type != VAL_AUTO && s->min_height.type != VAL_NONE) {
        float min_h = grid_resolve_len(s->min_height, fs, containing_height);
        if (box->rect.height < min_h) box->rect.height = min_h;
    }
    if (s->max_height.type != VAL_NONE && s->max_height.type != VAL_AUTO) {
        float max_h = grid_resolve_len(s->max_height, fs, containing_height);
        if (box->rect.height > max_h) box->rect.height = max_h;
    }
}
