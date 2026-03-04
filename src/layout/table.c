/*
 * Pane — Table Layout Implementation
 *
 * Implements basic table layout per CSS 2.1 §17:
 *   1. Collect rows and cells from the table box tree
 *   2. Determine column count and resolve column widths
 *   3. Lay out each row: position cells horizontally, run block layout per cell
 *   4. Equalize row heights across cells in the same row
 */

#include "table.h"
#include "block.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Maximum columns we support. */
#define MAX_COLS 64

/* ── Helpers ──────────────────────────────────────────────────────────── */

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

static bool is_table_row_group(LayoutBoxType t)
{
    return t == BOX_TABLE_ROW; /* row groups mapped to BOX_TABLE_ROW */
}

static bool is_table_row(LayoutBox *box)
{
    return box->type == BOX_TABLE_ROW;
}

static bool is_table_cell(LayoutBox *box)
{
    return box->type == BOX_TABLE_CELL;
}

/* Get the HTML "width" attribute value from a box's DOM node. */
static float get_attr_width(const LayoutBox *box, float containing_width)
{
    if (!box->node || box->node->type != PANE_NODE_ELEMENT) return -1;
    const char *w = elem_get_attr(box->node, "width");
    if (!w || !w[0]) return -1;

    size_t len = strlen(w);
    if (w[len - 1] == '%') {
        float pct = (float)atof(w);
        return pct * containing_width / 100.0f;
    }
    float px = (float)atof(w);
    return px > 0 ? px : -1;
}

/* Get cellpadding from table's DOM node. */
static float get_cellpadding(const LayoutBox *table)
{
    if (!table->node || table->node->type != PANE_NODE_ELEMENT) return 0;
    const char *cp = elem_get_attr(table->node, "cellpadding");
    if (!cp || !cp[0]) return 0;
    float v = (float)atof(cp);
    return v > 0 ? v : 0;
}

/* Get cellspacing from table's DOM node. */
static float get_cellspacing(const LayoutBox *table)
{
    if (!table->node || table->node->type != PANE_NODE_ELEMENT) return 2;
    const char *cs = elem_get_attr(table->node, "cellspacing");
    if (!cs || !cs[0]) return 2; /* default cellspacing is 2px */
    float v = (float)atof(cs);
    return v >= 0 ? v : 2;
}

/* ── Collect rows ─────────────────────────────────────────────────────── */

typedef struct {
    LayoutBox *rows[256];
    int        row_count;
} TableRows;

/* Recursively collect actual row boxes (skip row groups like THEAD/TBODY). */
static void collect_rows(LayoutBox *box, TableRows *out)
{
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        if (child->style && child->style->display == DISPLAY_NONE)
            continue;

        if (is_table_cell(child)) {
            /* Bare cell not in a row — treat the parent as a single implicit row. */
            if (out->row_count < 256)
                out->rows[out->row_count++] = box;
            return;
        }

        if (is_table_row(child)) {
            if (out->row_count < 256)
                out->rows[out->row_count++] = child;
        } else {
            /* Row group (THEAD, TBODY, TFOOT) — descend into it. */
            collect_rows(child, out);
        }
    }
}

/* Count cells in a row. */
static int count_cells(LayoutBox *row)
{
    int n = 0;
    for (LayoutBox *c = row->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;
        if (is_table_cell(c)) n++;
    }
    return n;
}

/* Get the nth cell in a row. */
static LayoutBox *get_cell(LayoutBox *row, int idx)
{
    int n = 0;
    for (LayoutBox *c = row->first_child; c; c = c->next_sibling) {
        if (c->style && c->style->display == DISPLAY_NONE) continue;
        if (is_table_cell(c)) {
            if (n == idx) return c;
            n++;
        }
    }
    return NULL;
}

/* ── Table Layout ─────────────────────────────────────────────────────── */

void layout_table(LayoutBox *box, float containing_width, float containing_height,
                  Arena *arena)
{
    ComputedStyle *s = box->style;
    float fs = s ? s->font_size : 16.0f;

    /* Resolve table edges. */
    box->margin = s ? s->margin : (EdgeSizes){0};
    box->padding = s ? s->padding : (EdgeSizes){0};
    box->border = s ? s->border_width : (EdgeSizes){0};

    /* Resolve table width. */
    float table_width;
    if (s && s->width.type != VAL_AUTO) {
        table_width = resolve_len(s->width, fs, containing_width);
    } else {
        /* Check HTML width attribute. */
        float attr_w = get_attr_width(box, containing_width);
        if (attr_w > 0) {
            table_width = attr_w;
        } else {
            /* Auto: fill containing block. */
            table_width = containing_width
                - box->margin.left - box->margin.right
                - box->border.left - box->border.right
                - box->padding.left - box->padding.right;
        }
    }
    if (table_width < 0) table_width = 0;
    box->rect.width = table_width;

    float cellpadding = get_cellpadding(box);
    float cellspacing = get_cellspacing(box);

    /* Collect all rows. */
    TableRows trows = {0};
    collect_rows(box, &trows);

    if (trows.row_count == 0) {
        box->rect.height = 0;
        return;
    }

    /* Determine column count (max cells across all rows). */
    int num_cols = 0;
    for (int r = 0; r < trows.row_count; r++) {
        int nc = count_cells(trows.rows[r]);
        if (nc > num_cols) num_cols = nc;
    }
    if (num_cols > MAX_COLS) num_cols = MAX_COLS;
    if (num_cols == 0) {
        box->rect.height = 0;
        return;
    }

    /* Available width for cells (after spacing). */
    float avail_width = table_width
        - cellspacing * (float)(num_cols + 1)
        - box->padding.left - box->padding.right;
    if (avail_width < 0) avail_width = 0;

    /* ── Phase 1: Determine column widths ──────────────────────────── */

    float col_widths[MAX_COLS] = {0};
    bool col_has_width[MAX_COLS] = {false};
    float total_specified = 0;
    int unspecified_count = 0;

    /* First pass: check for explicit widths on cells (HTML width attr or CSS). */
    for (int r = 0; r < trows.row_count; r++) {
        int nc = count_cells(trows.rows[r]);
        for (int c = 0; c < nc && c < num_cols; c++) {
            LayoutBox *cell = get_cell(trows.rows[r], c);
            if (!cell) continue;

            float w = -1;

            /* CSS width. */
            if (cell->style && cell->style->width.type != VAL_AUTO) {
                w = resolve_len(cell->style->width, cell->style->font_size, table_width);
            }

            /* HTML width attribute (lower priority if CSS already set). */
            if (w < 0) {
                w = get_attr_width(cell, table_width);
            }

            if (w > 0 && w > col_widths[c]) {
                col_widths[c] = w;
                col_has_width[c] = true;
            }
        }
    }

    /* Account for cellpadding in specified widths. */
    for (int c = 0; c < num_cols; c++) {
        if (col_has_width[c]) {
            /* If width includes padding, subtract it for content width. */
            total_specified += col_widths[c];
        } else {
            unspecified_count++;
        }
    }

    /* Distribute remaining width to unspecified columns. */
    if (unspecified_count > 0) {
        float remaining = avail_width - total_specified;
        if (remaining < 0) remaining = 0;
        float per_col = remaining / (float)unspecified_count;
        for (int c = 0; c < num_cols; c++) {
            if (!col_has_width[c]) {
                col_widths[c] = per_col;
            }
        }
    } else {
        /* All columns have specified widths. Scale to fit table if needed. */
        if (total_specified > 0 && total_specified != avail_width) {
            float scale = avail_width / total_specified;
            for (int c = 0; c < num_cols; c++) {
                col_widths[c] *= scale;
            }
        }
    }

    /* ── Phase 2: Lay out rows and cells ───────────────────────────── */

    float cursor_y = box->padding.top + cellspacing;

    for (int r = 0; r < trows.row_count; r++) {
        LayoutBox *row = trows.rows[r];
        int nc = count_cells(row);

        /* Position each cell. */
        float cursor_x = box->padding.left + cellspacing;
        float row_height = 0;

        for (int c = 0; c < nc && c < num_cols; c++) {
            LayoutBox *cell = get_cell(row, c);
            if (!cell) continue;

            /* Apply cellpadding if cell has no explicit padding. */
            if (cellpadding > 0 && cell->style) {
                if (cell->padding.top == 0 && cell->style->padding.top == 0)
                    cell->padding.top = cellpadding;
                if (cell->padding.bottom == 0 && cell->style->padding.bottom == 0)
                    cell->padding.bottom = cellpadding;
                if (cell->padding.left == 0 && cell->style->padding.left == 0)
                    cell->padding.left = cellpadding;
                if (cell->padding.right == 0 && cell->style->padding.right == 0)
                    cell->padding.right = cellpadding;
            }

            float cell_width = col_widths[c];

            /* Lay out cell content as a block. */
            layout_block(cell, cell_width, containing_height, arena);

            /* Override the cell width to match column width. */
            cell->rect.width = cell_width
                - cell->padding.left - cell->padding.right
                - cell->border.left - cell->border.right;
            if (cell->rect.width < 0) cell->rect.width = 0;

            cell->rect.x = cursor_x + cell->margin.left + cell->border.left + cell->padding.left;
            cell->rect.y = cursor_y + cell->margin.top + cell->border.top + cell->padding.top;

            float cell_outer_h = cell->rect.height
                + cell->padding.top + cell->padding.bottom
                + cell->border.top + cell->border.bottom
                + cell->margin.top + cell->margin.bottom;
            if (cell_outer_h > row_height) row_height = cell_outer_h;

            cursor_x += col_widths[c] + cellspacing;
        }

        /* Position the row box itself. */
        row->rect.x = 0;
        row->rect.y = cursor_y;
        row->rect.width = table_width;
        row->rect.height = row_height;
        row->margin = (EdgeSizes){0};
        row->border = (EdgeSizes){0};
        row->padding = (EdgeSizes){0};

        /* Equalize cell heights within this row (valign: top by default). */
        for (int c = 0; c < nc && c < num_cols; c++) {
            LayoutBox *cell = get_cell(row, c);
            if (!cell) continue;

            float cell_content_h = row_height
                - cell->padding.top - cell->padding.bottom
                - cell->border.top - cell->border.bottom
                - cell->margin.top - cell->margin.bottom;
            if (cell_content_h > cell->rect.height)
                cell->rect.height = cell_content_h;
        }

        cursor_y += row_height + cellspacing;
    }

    /* Set table height. */
    float content_h = cursor_y + box->padding.bottom;
    if (s && s->height.type != VAL_AUTO) {
        float specified_h = resolve_len(s->height, fs, containing_height);
        if (specified_h > content_h) content_h = specified_h;
    }
    box->rect.height = content_h;
}
