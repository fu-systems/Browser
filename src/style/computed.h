/*
 * Pane — Computed Style
 *
 * Resolved style values for a DOM element. Combines cascaded values,
 * inheritance, and initial values into final computed values.
 */

#ifndef PANE_COMPUTED_H
#define PANE_COMPUTED_H

#include "../css/properties.h"
#include "../css/values.h"
#include "../css/cascade.h"
#include "../dom/dom.h"
#include "../util/arena.h"

/* ── Display Values ────────────────────────────────────────────────── */

typedef enum {
    DISPLAY_NONE,
    DISPLAY_BLOCK,
    DISPLAY_INLINE,
    DISPLAY_INLINE_BLOCK,
    DISPLAY_FLEX,
    DISPLAY_INLINE_FLEX,
    DISPLAY_GRID,
    DISPLAY_INLINE_GRID,
    DISPLAY_TABLE,
    DISPLAY_INLINE_TABLE,
    DISPLAY_TABLE_ROW_GROUP,
    DISPLAY_TABLE_HEADER_GROUP,
    DISPLAY_TABLE_FOOTER_GROUP,
    DISPLAY_TABLE_ROW,
    DISPLAY_TABLE_CELL,
    DISPLAY_TABLE_COLUMN,
    DISPLAY_TABLE_COLUMN_GROUP,
    DISPLAY_TABLE_CAPTION,
    DISPLAY_LIST_ITEM,
    DISPLAY_FLOW_ROOT,
    DISPLAY_CONTENTS,
} Display;

/* ── Position Values ───────────────────────────────────────────────── */

typedef enum {
    POSITION_STATIC,
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
    POSITION_FIXED,
    POSITION_STICKY,
} Position;

/* ── Float Values ──────────────────────────────────────────────────── */

typedef enum {
    FLOAT_NONE,
    FLOAT_LEFT,
    FLOAT_RIGHT,
} Float;

/* ── Clear Values ─────────────────────────────────────────────────── */

typedef enum {
    CLEAR_NONE,
    CLEAR_LEFT,
    CLEAR_RIGHT,
    CLEAR_BOTH,
} Clear;

/* ── Writing Mode ──────────────────────────────────────────────────── */

typedef enum {
    WM_HORIZONTAL_TB,
    WM_VERTICAL_RL,
    WM_VERTICAL_LR,
} WritingMode;

/* ── Direction ─────────────────────────────────────────────────────── */

typedef enum {
    DIR_LTR,
    DIR_RTL,
} Direction;

/* ── Overflow ──────────────────────────────────────────────────────── */

typedef enum {
    OVERFLOW_VISIBLE,
    OVERFLOW_HIDDEN,
    OVERFLOW_SCROLL,
    OVERFLOW_AUTO,
} Overflow;

/* ── Visibility ────────────────────────────────────────────────────── */

typedef enum {
    VIS_VISIBLE,
    VIS_HIDDEN,
    VIS_COLLAPSE,
} Visibility;

/* ── Text Align ────────────────────────────────────────────────────── */

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_JUSTIFY,
    TEXT_ALIGN_START,
    TEXT_ALIGN_END,
} TextAlign;

/* ── White Space ───────────────────────────────────────────────────── */

typedef enum {
    WS_NORMAL,
    WS_NOWRAP,
    WS_PRE,
    WS_PRE_WRAP,
    WS_PRE_LINE,
    WS_BREAK_SPACES,
} WhiteSpace;

/* ── Flex Direction ────────────────────────────────────────────────── */

typedef enum {
    FLEXDIR_ROW,
    FLEXDIR_ROW_REVERSE,
    FLEXDIR_COLUMN,
    FLEXDIR_COLUMN_REVERSE,
} FlexDirection;

/* ── Flex Wrap ─────────────────────────────────────────────────────── */

typedef enum {
    FLEXWRAP_NOWRAP,
    FLEXWRAP_WRAP,
    FLEXWRAP_WRAP_REVERSE,
} FlexWrap;

/* ── Grid Auto Flow ────────────────────────────────────────────────── */

typedef enum {
    GRID_FLOW_ROW,
    GRID_FLOW_COLUMN,
    GRID_FLOW_ROW_DENSE,
    GRID_FLOW_COLUMN_DENSE,
} GridAutoFlow;

/* ── Box Sizing ────────────────────────────────────────────────────── */

typedef enum {
    BOX_CONTENT_BOX,
    BOX_BORDER_BOX,
} BoxSizing;

/* ── Border Style ──────────────────────────────────────────────────── */

typedef enum {
    BORDER_NONE,
    BORDER_HIDDEN,
    BORDER_SOLID,
    BORDER_DASHED,
    BORDER_DOTTED,
    BORDER_DOUBLE,
    BORDER_GROOVE,
    BORDER_RIDGE,
    BORDER_INSET,
    BORDER_OUTSET,
} BorderStyle;

/* ── Edge Values ───────────────────────────────────────────────────── */

typedef struct {
    float top, right, bottom, left;
} EdgeSizes;

/* ── Computed Style ────────────────────────────────────────────────── */

typedef struct ComputedStyle ComputedStyle;
struct ComputedStyle {
    /* Display & positioning */
    Display      display;
    Position     position;
    Float        float_val;
    Clear        clear_val;

    /* Box model */
    CssValue     width, height;
    CssValue     min_width, min_height;
    CssValue     max_width, max_height;
    EdgeSizes    margin;
    EdgeSizes    padding;
    EdgeSizes    border_width;
    CssValue     top, right, bottom, left;
    BoxSizing    box_sizing;

    /* Margin auto flags (for centering) */
    bool         margin_top_auto;
    bool         margin_right_auto;
    bool         margin_bottom_auto;
    bool         margin_left_auto;

    /* Border style & color */
    BorderStyle  border_top_style, border_right_style;
    BorderStyle  border_bottom_style, border_left_style;
    CssColor     border_top_color, border_right_color;
    CssColor     border_bottom_color, border_left_color;

    /* Background */
    CssColor     background_color;

    /* Text & font (inherited) */
    float        font_size;
    float        line_height;
    CssColor     color;
    TextAlign    text_align;
    WhiteSpace   white_space;
    const char  *font_family;
    int          font_weight;

    /* Visual */
    Overflow     overflow_x, overflow_y;
    Visibility   visibility;
    float        opacity;

    /* Writing mode (inherited) */
    WritingMode  writing_mode;
    Direction    direction;

    /* Flexbox */
    FlexDirection flex_direction;
    FlexWrap      flex_wrap;
    float         flex_grow;
    float         flex_shrink;
    CssValue      flex_basis;
    CssValue      align_items;
    CssValue      align_self;
    CssValue      justify_content;

    /* Grid */
    CssValue     grid_template_columns;  /* VAL_NONE or VAL_LIST of track sizes */
    CssValue     grid_template_rows;     /* VAL_NONE or VAL_LIST of track sizes */
    CssValue     grid_auto_columns;      /* auto track size */
    CssValue     grid_auto_rows;         /* auto track size */
    CssValue     grid_column_start;      /* line number or auto */
    CssValue     grid_column_end;        /* line number or auto */
    CssValue     grid_row_start;         /* line number or auto */
    CssValue     grid_row_end;           /* line number or auto */
    CssValue     row_gap;
    CssValue     column_gap;
    CssValue     justify_items;
    CssValue     align_content;
    GridAutoFlow grid_auto_flow;

    /* Z-index */
    CssValue     z_index;

    /* Transform */
    bool         has_transform;

    /* Raw property values (for properties not given explicit fields). */
    CssValue     values[CSS_PROP__COUNT];
};

/* ── API ────────────────────────────────────────────────────────────── */

/* Create a computed style with initial values. */
ComputedStyle *computed_style_create(Arena *arena);

/* Resolve a computed style from cascade result + parent style. */
void computed_style_resolve(ComputedStyle *style,
                            const CascadeResult *cascade,
                            const ComputedStyle *parent,
                            float root_font_size);

/* Get the default computed style (UA defaults + initial values). */
const ComputedStyle *computed_style_initial(void);

/* Get the default display for an HTML tag. */
Display display_for_tag(HtmlTag tag);

#endif /* PANE_COMPUTED_H */
