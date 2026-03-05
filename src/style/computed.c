/*
 * Pane — Computed Style Implementation
 */

#include "computed.h"
#include <string.h>
#include <math.h>

/* ── Initial Style ─────────────────────────────────────────────────── */

static ComputedStyle initial_style = {
    .display        = DISPLAY_INLINE,
    .position       = POSITION_STATIC,
    .float_val      = FLOAT_NONE,
    .width          = { .type = VAL_AUTO },
    .height         = { .type = VAL_AUTO },
    .min_width      = { .type = VAL_AUTO },
    .min_height     = { .type = VAL_AUTO },
    .max_width      = { .type = VAL_NONE },
    .max_height     = { .type = VAL_NONE },
    .margin         = {0},
    .padding        = {0},
    .border_width   = {0},
    .top            = { .type = VAL_AUTO },
    .right          = { .type = VAL_AUTO },
    .bottom         = { .type = VAL_AUTO },
    .left           = { .type = VAL_AUTO },
    .box_sizing     = BOX_CONTENT_BOX,
    .border_top_style    = BORDER_NONE,
    .border_right_style  = BORDER_NONE,
    .border_bottom_style = BORDER_NONE,
    .border_left_style   = BORDER_NONE,
    .background_color = {0, 0, 0, 0},
    .font_size      = 16.0f,
    .line_height    = 1.2f,
    .color          = {0, 0, 0, 255},
    .text_align     = TEXT_ALIGN_START,
    .white_space    = WS_NORMAL,
    .font_family    = "serif",
    .font_weight    = 400,
    .overflow_x     = OVERFLOW_VISIBLE,
    .overflow_y     = OVERFLOW_VISIBLE,
    .visibility     = VIS_VISIBLE,
    .opacity        = 1.0f,
    .writing_mode   = WM_HORIZONTAL_TB,
    .direction      = DIR_LTR,
    .flex_direction = FLEXDIR_ROW,
    .flex_wrap      = FLEXWRAP_NOWRAP,
    .flex_grow      = 0.0f,
    .flex_shrink    = 1.0f,
    .flex_basis     = { .type = VAL_AUTO },
    .align_items    = { .type = VAL_KEYWORD },
    .align_self     = { .type = VAL_AUTO },
    .justify_content = { .type = VAL_KEYWORD },
    .grid_template_columns = { .type = VAL_NONE },
    .grid_template_rows    = { .type = VAL_NONE },
    .grid_auto_columns     = { .type = VAL_AUTO },
    .grid_auto_rows        = { .type = VAL_AUTO },
    .grid_column_start     = { .type = VAL_AUTO },
    .grid_column_end       = { .type = VAL_AUTO },
    .grid_row_start        = { .type = VAL_AUTO },
    .grid_row_end          = { .type = VAL_AUTO },
    .row_gap               = { .type = VAL_NONE },
    .column_gap            = { .type = VAL_NONE },
    .justify_items         = { .type = VAL_KEYWORD },
    .align_content         = { .type = VAL_KEYWORD },
    .grid_auto_flow        = GRID_FLOW_ROW,
    .z_index        = { .type = VAL_AUTO },
    .has_transform  = false,
};

const ComputedStyle *computed_style_initial(void)
{
    return &initial_style;
}

ComputedStyle *computed_style_create(Arena *arena)
{
    ComputedStyle *s = arena_alloc(arena, sizeof(ComputedStyle), 8);
    *s = initial_style;
    return s;
}

/* ── Tag defaults ──────────────────────────────────────────────────── */

Display display_for_tag(HtmlTag tag)
{
    switch (tag) {
    /* Block elements */
    case TAG_HTML: case TAG_BODY: case TAG_DIV: case TAG_P:
    case TAG_H1: case TAG_H2: case TAG_H3: case TAG_H4: case TAG_H5: case TAG_H6:
    case TAG_UL: case TAG_OL: case TAG_LI: case TAG_DL: case TAG_DT: case TAG_DD:
    case TAG_BLOCKQUOTE: case TAG_PRE: case TAG_FIGURE: case TAG_FIGCAPTION:
    case TAG_MAIN: case TAG_HEADER: case TAG_FOOTER: case TAG_NAV:
    case TAG_SECTION: case TAG_ARTICLE: case TAG_ASIDE: case TAG_HGROUP:
    case TAG_ADDRESS: case TAG_SEARCH: case TAG_HR: case TAG_FIELDSET:
    case TAG_LEGEND: case TAG_DETAILS: case TAG_SUMMARY: case TAG_DIALOG:
    case TAG_FORM: case TAG_CENTER: case TAG_DIR: case TAG_LISTING:
    case TAG_MENU:
        return DISPLAY_BLOCK;

    /* Table elements */
    case TAG_TABLE:    return DISPLAY_TABLE;
    case TAG_THEAD:    return DISPLAY_TABLE_HEADER_GROUP;
    case TAG_TBODY:    return DISPLAY_TABLE_ROW_GROUP;
    case TAG_TFOOT:    return DISPLAY_TABLE_FOOTER_GROUP;
    case TAG_TR:       return DISPLAY_TABLE_ROW;
    case TAG_TD:       return DISPLAY_TABLE_CELL;
    case TAG_TH:       return DISPLAY_TABLE_CELL;
    case TAG_CAPTION:  return DISPLAY_TABLE_CAPTION;
    case TAG_COLGROUP: return DISPLAY_TABLE_COLUMN_GROUP;
    case TAG_COL:      return DISPLAY_TABLE_COLUMN;

    /* None */
    case TAG_HEAD: case TAG_TITLE: case TAG_META: case TAG_LINK:
    case TAG_STYLE: case TAG_SCRIPT: case TAG_NOSCRIPT:
    case TAG_TEMPLATE: case TAG_NOFRAMES: case TAG_NOEMBED:
        return DISPLAY_NONE;

    /* List items */
    /* LI is already block above; browsers use display:list-item. */

    default:
        return DISPLAY_INLINE;
    }
}

/* ── Value helpers ─────────────────────────────────────────────────── */

static float resolve_length(CssValue val, float font_size, float root_fs,
                            float containing)
{
    if (val.type == VAL_LENGTH)
        return css_length_to_px(val, font_size, root_fs, 0, 0, containing);
    if (val.type == VAL_PERCENTAGE)
        return val.percentage * containing / 100.0f;
    if (val.type == VAL_NUMBER)
        return val.number;
    return 0.0f;
}

static Display parse_display(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD) return DISPLAY_INLINE;
    const char *s = val->string;
    if (!s) return DISPLAY_INLINE;

    if (strcmp(s, "none") == 0)         return DISPLAY_NONE;
    if (strcmp(s, "block") == 0)        return DISPLAY_BLOCK;
    if (strcmp(s, "inline") == 0)       return DISPLAY_INLINE;
    if (strcmp(s, "inline-block") == 0) return DISPLAY_INLINE_BLOCK;
    if (strcmp(s, "flex") == 0)         return DISPLAY_FLEX;
    if (strcmp(s, "inline-flex") == 0)  return DISPLAY_INLINE_FLEX;
    if (strcmp(s, "grid") == 0)         return DISPLAY_GRID;
    if (strcmp(s, "inline-grid") == 0)  return DISPLAY_INLINE_GRID;
    if (strcmp(s, "table") == 0)        return DISPLAY_TABLE;
    if (strcmp(s, "inline-table") == 0) return DISPLAY_INLINE_TABLE;
    if (strcmp(s, "table-row") == 0)    return DISPLAY_TABLE_ROW;
    if (strcmp(s, "table-cell") == 0)   return DISPLAY_TABLE_CELL;
    if (strcmp(s, "table-row-group") == 0)    return DISPLAY_TABLE_ROW_GROUP;
    if (strcmp(s, "table-header-group") == 0) return DISPLAY_TABLE_HEADER_GROUP;
    if (strcmp(s, "table-footer-group") == 0) return DISPLAY_TABLE_FOOTER_GROUP;
    if (strcmp(s, "table-column") == 0)       return DISPLAY_TABLE_COLUMN;
    if (strcmp(s, "table-column-group") == 0) return DISPLAY_TABLE_COLUMN_GROUP;
    if (strcmp(s, "table-caption") == 0)      return DISPLAY_TABLE_CAPTION;
    if (strcmp(s, "list-item") == 0)    return DISPLAY_LIST_ITEM;
    if (strcmp(s, "flow-root") == 0)    return DISPLAY_FLOW_ROOT;
    if (strcmp(s, "contents") == 0)     return DISPLAY_CONTENTS;
    return DISPLAY_INLINE;
}

static Position parse_position(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return POSITION_STATIC;
    const char *s = val->string;
    if (strcmp(s, "relative") == 0) return POSITION_RELATIVE;
    if (strcmp(s, "absolute") == 0) return POSITION_ABSOLUTE;
    if (strcmp(s, "fixed") == 0)    return POSITION_FIXED;
    if (strcmp(s, "sticky") == 0)   return POSITION_STICKY;
    return POSITION_STATIC;
}

static Float parse_float(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return FLOAT_NONE;
    const char *s = val->string;
    if (strcmp(s, "left") == 0)  return FLOAT_LEFT;
    if (strcmp(s, "right") == 0) return FLOAT_RIGHT;
    return FLOAT_NONE;
}

static Overflow parse_overflow(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return OVERFLOW_VISIBLE;
    const char *s = val->string;
    if (strcmp(s, "hidden") == 0) return OVERFLOW_HIDDEN;
    if (strcmp(s, "scroll") == 0) return OVERFLOW_SCROLL;
    if (strcmp(s, "auto") == 0)   return OVERFLOW_AUTO;
    return OVERFLOW_VISIBLE;
}

static Visibility parse_visibility(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return VIS_VISIBLE;
    const char *s = val->string;
    if (strcmp(s, "hidden") == 0)   return VIS_HIDDEN;
    if (strcmp(s, "collapse") == 0) return VIS_COLLAPSE;
    return VIS_VISIBLE;
}

static TextAlign parse_text_align(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return TEXT_ALIGN_START;
    const char *s = val->string;
    if (strcmp(s, "left") == 0)    return TEXT_ALIGN_LEFT;
    if (strcmp(s, "right") == 0)   return TEXT_ALIGN_RIGHT;
    if (strcmp(s, "center") == 0)  return TEXT_ALIGN_CENTER;
    if (strcmp(s, "justify") == 0) return TEXT_ALIGN_JUSTIFY;
    if (strcmp(s, "end") == 0)     return TEXT_ALIGN_END;
    return TEXT_ALIGN_START;
}

static WhiteSpace parse_white_space(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return WS_NORMAL;
    const char *s = val->string;
    if (strcmp(s, "nowrap") == 0)       return WS_NOWRAP;
    if (strcmp(s, "pre") == 0)         return WS_PRE;
    if (strcmp(s, "pre-wrap") == 0)    return WS_PRE_WRAP;
    if (strcmp(s, "pre-line") == 0)    return WS_PRE_LINE;
    if (strcmp(s, "break-spaces") == 0) return WS_BREAK_SPACES;
    return WS_NORMAL;
}

static BoxSizing parse_box_sizing(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return BOX_CONTENT_BOX;
    if (strcmp(val->string, "border-box") == 0) return BOX_BORDER_BOX;
    return BOX_CONTENT_BOX;
}

static BorderStyle parse_border_style(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return BORDER_NONE;
    const char *s = val->string;
    if (strcmp(s, "solid") == 0)  return BORDER_SOLID;
    if (strcmp(s, "dashed") == 0) return BORDER_DASHED;
    if (strcmp(s, "dotted") == 0) return BORDER_DOTTED;
    if (strcmp(s, "double") == 0) return BORDER_DOUBLE;
    if (strcmp(s, "groove") == 0) return BORDER_GROOVE;
    if (strcmp(s, "ridge") == 0)  return BORDER_RIDGE;
    if (strcmp(s, "inset") == 0)  return BORDER_INSET;
    if (strcmp(s, "outset") == 0) return BORDER_OUTSET;
    if (strcmp(s, "hidden") == 0) return BORDER_HIDDEN;
    return BORDER_NONE;
}

static FlexDirection parse_flex_direction(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return FLEXDIR_ROW;
    const char *s = val->string;
    if (strcmp(s, "row-reverse") == 0)    return FLEXDIR_ROW_REVERSE;
    if (strcmp(s, "column") == 0)         return FLEXDIR_COLUMN;
    if (strcmp(s, "column-reverse") == 0) return FLEXDIR_COLUMN_REVERSE;
    return FLEXDIR_ROW;
}

static FlexWrap parse_flex_wrap(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return FLEXWRAP_NOWRAP;
    const char *s = val->string;
    if (strcmp(s, "wrap") == 0)         return FLEXWRAP_WRAP;
    if (strcmp(s, "wrap-reverse") == 0) return FLEXWRAP_WRAP_REVERSE;
    return FLEXWRAP_NOWRAP;
}

static GridAutoFlow parse_grid_auto_flow(const CssValue *val)
{
    if (!val || val->type != VAL_KEYWORD || !val->string) return GRID_FLOW_ROW;
    const char *s = val->string;
    if (strcmp(s, "column") == 0)       return GRID_FLOW_COLUMN;
    if (strcmp(s, "row dense") == 0)    return GRID_FLOW_ROW_DENSE;
    if (strcmp(s, "column dense") == 0) return GRID_FLOW_COLUMN_DENSE;
    if (strcmp(s, "dense") == 0)        return GRID_FLOW_ROW_DENSE;
    return GRID_FLOW_ROW;
}

static CssColor resolve_color(const CssValue *val, CssColor inherited)
{
    if (!val) return inherited;
    if (val->type == VAL_COLOR) return val->color;
    if (val->type == VAL_KEYWORD && val->string &&
        strcmp(val->string, "currentcolor") == 0)
        return inherited;
    if (val->type == VAL_INHERIT) return inherited;
    return inherited;
}

static float resolve_font_size(const CssValue *val, float parent_fs,
                                float root_fs)
{
    if (!val) return parent_fs;
    switch (val->type) {
    case VAL_LENGTH:
        return css_length_to_px(*val, parent_fs, root_fs, 0, 0, 0);
    case VAL_PERCENTAGE:
        return parent_fs * val->percentage / 100.0f;
    case VAL_NUMBER:
        return val->number;
    case VAL_KEYWORD:
        if (val->string) {
            if (strcmp(val->string, "larger") == 0) return parent_fs * 1.2f;
            if (strcmp(val->string, "smaller") == 0) return parent_fs / 1.2f;
            if (strcmp(val->string, "xx-small") == 0) return 9.0f;
            if (strcmp(val->string, "x-small") == 0)  return 10.0f;
            if (strcmp(val->string, "small") == 0)    return 13.0f;
            if (strcmp(val->string, "medium") == 0)   return 16.0f;
            if (strcmp(val->string, "large") == 0)    return 18.0f;
            if (strcmp(val->string, "x-large") == 0)  return 24.0f;
            if (strcmp(val->string, "xx-large") == 0) return 32.0f;
        }
        return parent_fs;
    case VAL_INHERIT:
        return parent_fs;
    default:
        return parent_fs;
    }
}

static int parse_font_weight(const CssValue *val, int parent_weight)
{
    if (!val) return parent_weight;
    if (val->type == VAL_NUMBER) return (int)val->number;
    if (val->type == VAL_KEYWORD && val->string) {
        if (strcmp(val->string, "normal") == 0) return 400;
        if (strcmp(val->string, "bold") == 0)   return 700;
        if (strcmp(val->string, "bolder") == 0) {
            if (parent_weight < 400) return 400;
            if (parent_weight < 600) return 700;
            return 900;
        }
        if (strcmp(val->string, "lighter") == 0) {
            if (parent_weight < 600) return 100;
            if (parent_weight < 800) return 400;
            return 700;
        }
    }
    if (val->type == VAL_INHERIT) return parent_weight;
    return parent_weight;
}

/* ── Main resolve function ─────────────────────────────────────────── */

void computed_style_resolve(ComputedStyle *style,
                            const CascadeResult *cascade,
                            const ComputedStyle *parent,
                            float root_font_size)
{
    if (!parent) parent = &initial_style;

    /* Start with inherited properties from parent. */
    style->color        = parent->color;
    style->font_size    = parent->font_size;
    style->line_height  = parent->line_height;
    style->font_family  = parent->font_family;
    style->font_weight  = parent->font_weight;
    style->text_align   = parent->text_align;
    style->white_space  = parent->white_space;
    style->visibility   = parent->visibility;
    style->writing_mode = parent->writing_mode;
    style->direction    = parent->direction;

    /* Non-inherited defaults. */
    style->display       = DISPLAY_INLINE;
    style->position      = POSITION_STATIC;
    style->float_val     = FLOAT_NONE;
    style->width         = (CssValue){ .type = VAL_AUTO };
    style->height        = (CssValue){ .type = VAL_AUTO };
    style->min_width     = (CssValue){ .type = VAL_AUTO };
    style->min_height    = (CssValue){ .type = VAL_AUTO };
    style->max_width     = (CssValue){ .type = VAL_NONE };
    style->max_height    = (CssValue){ .type = VAL_NONE };
    style->margin        = (EdgeSizes){0};
    style->padding       = (EdgeSizes){0};
    style->border_width  = (EdgeSizes){0};
    style->top           = (CssValue){ .type = VAL_AUTO };
    style->right         = (CssValue){ .type = VAL_AUTO };
    style->bottom        = (CssValue){ .type = VAL_AUTO };
    style->left          = (CssValue){ .type = VAL_AUTO };
    style->box_sizing    = BOX_CONTENT_BOX;
    style->overflow_x    = OVERFLOW_VISIBLE;
    style->overflow_y    = OVERFLOW_VISIBLE;
    style->opacity       = 1.0f;
    style->background_color = CSS_COLOR_TRANSPARENT;
    style->flex_direction = FLEXDIR_ROW;
    style->flex_wrap      = FLEXWRAP_NOWRAP;
    style->flex_grow      = 0.0f;
    style->flex_shrink    = 1.0f;
    style->flex_basis     = (CssValue){ .type = VAL_AUTO };
    style->grid_template_columns = (CssValue){ .type = VAL_NONE };
    style->grid_template_rows    = (CssValue){ .type = VAL_NONE };
    style->grid_auto_columns     = (CssValue){ .type = VAL_AUTO };
    style->grid_auto_rows        = (CssValue){ .type = VAL_AUTO };
    style->grid_column_start     = (CssValue){ .type = VAL_AUTO };
    style->grid_column_end       = (CssValue){ .type = VAL_AUTO };
    style->grid_row_start        = (CssValue){ .type = VAL_AUTO };
    style->grid_row_end          = (CssValue){ .type = VAL_AUTO };
    style->row_gap               = (CssValue){ .type = VAL_NONE };
    style->column_gap            = (CssValue){ .type = VAL_NONE };
    style->justify_items         = (CssValue){ .type = VAL_KEYWORD };
    style->align_content         = (CssValue){ .type = VAL_KEYWORD };
    style->grid_auto_flow        = GRID_FLOW_ROW;
    style->z_index        = (CssValue){ .type = VAL_AUTO };
    style->has_transform  = false;
    style->border_top_style = style->border_right_style =
        style->border_bottom_style = style->border_left_style = BORDER_NONE;

    if (!cascade) return;

    float parent_fs = parent->font_size;
    float fs = parent_fs;

    /* First pass: resolve font-size (needed for em units). */
    const CssValue *fs_val = cascade_get(cascade, CSS_PROP_FONT_SIZE);
    if (fs_val) {
        fs = resolve_font_size(fs_val, parent_fs, root_font_size);
        style->font_size = fs;
    }

    /* Apply cascaded values. */
    for (int i = 0; i < cascade->count; i++) {
        CssPropId prop = cascade->decls[i].property;
        const CssValue *val = &cascade->decls[i].value;

        /* Handle inherit/initial for all properties. */
        if (val->type == VAL_INITIAL) continue;  /* already set to initial */
        if (val->type == VAL_INHERIT) {
            /* Copy from parent — handled per-property below. */
        }

        switch (prop) {
        case CSS_PROP_DISPLAY:
            if (val->type == VAL_NONE) style->display = DISPLAY_NONE;
            else style->display = parse_display(val);
            break;
        case CSS_PROP_POSITION:
            style->position = parse_position(val);
            break;
        case CSS_PROP_FLOAT:
            style->float_val = parse_float(val);
            break;
        case CSS_PROP_WIDTH:
            style->width = *val;
            break;
        case CSS_PROP_HEIGHT:
            style->height = *val;
            break;
        case CSS_PROP_MIN_WIDTH:
            style->min_width = *val;
            break;
        case CSS_PROP_MIN_HEIGHT:
            style->min_height = *val;
            break;
        case CSS_PROP_MAX_WIDTH:
            style->max_width = *val;
            break;
        case CSS_PROP_MAX_HEIGHT:
            style->max_height = *val;
            break;
        case CSS_PROP_MARGIN_TOP:
            style->margin_top_auto = (val->type == VAL_AUTO);
            style->margin.top = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_MARGIN_RIGHT:
            style->margin_right_auto = (val->type == VAL_AUTO);
            style->margin.right = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_MARGIN_BOTTOM:
            style->margin_bottom_auto = (val->type == VAL_AUTO);
            style->margin.bottom = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_MARGIN_LEFT:
            style->margin_left_auto = (val->type == VAL_AUTO);
            style->margin.left = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_PADDING_TOP:
            style->padding.top = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_PADDING_RIGHT:
            style->padding.right = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_PADDING_BOTTOM:
            style->padding.bottom = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_PADDING_LEFT:
            style->padding.left = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_BORDER_TOP_WIDTH:
            style->border_width.top = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_BORDER_RIGHT_WIDTH:
            style->border_width.right = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_BORDER_BOTTOM_WIDTH:
            style->border_width.bottom = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_BORDER_LEFT_WIDTH:
            style->border_width.left = resolve_length(*val, fs, root_font_size, 0);
            break;
        case CSS_PROP_BORDER_TOP_STYLE:
            style->border_top_style = parse_border_style(val);
            break;
        case CSS_PROP_BORDER_RIGHT_STYLE:
            style->border_right_style = parse_border_style(val);
            break;
        case CSS_PROP_BORDER_BOTTOM_STYLE:
            style->border_bottom_style = parse_border_style(val);
            break;
        case CSS_PROP_BORDER_LEFT_STYLE:
            style->border_left_style = parse_border_style(val);
            break;
        case CSS_PROP_BORDER_TOP_COLOR:
            style->border_top_color = resolve_color(val, style->color);
            break;
        case CSS_PROP_BORDER_RIGHT_COLOR:
            style->border_right_color = resolve_color(val, style->color);
            break;
        case CSS_PROP_BORDER_BOTTOM_COLOR:
            style->border_bottom_color = resolve_color(val, style->color);
            break;
        case CSS_PROP_BORDER_LEFT_COLOR:
            style->border_left_color = resolve_color(val, style->color);
            break;
        case CSS_PROP_BACKGROUND_COLOR:
            style->background_color = resolve_color(val, style->color);
            break;
        case CSS_PROP_COLOR:
            style->color = resolve_color(val, parent->color);
            break;
        case CSS_PROP_FONT_SIZE:
            /* Already resolved above. */
            break;
        case CSS_PROP_LINE_HEIGHT:
            if (val->type == VAL_NUMBER)
                style->line_height = val->number;
            else if (val->type == VAL_LENGTH || val->type == VAL_PERCENTAGE)
                style->line_height = resolve_length(*val, fs, root_font_size, 0) / fs;
            break;
        case CSS_PROP_FONT_FAMILY:
            if (val->type == VAL_STRING || val->type == VAL_KEYWORD)
                style->font_family = val->string;
            break;
        case CSS_PROP_FONT_WEIGHT:
            style->font_weight = parse_font_weight(val, parent->font_weight);
            break;
        case CSS_PROP_TEXT_ALIGN:
            style->text_align = parse_text_align(val);
            break;
        case CSS_PROP_WHITE_SPACE:
            style->white_space = parse_white_space(val);
            break;
        case CSS_PROP_BOX_SIZING:
            style->box_sizing = parse_box_sizing(val);
            break;
        case CSS_PROP_OVERFLOW_X:
        case CSS_PROP_OVERFLOW:
            style->overflow_x = parse_overflow(val);
            if (prop == CSS_PROP_OVERFLOW)
                style->overflow_y = style->overflow_x;
            break;
        case CSS_PROP_OVERFLOW_Y:
            style->overflow_y = parse_overflow(val);
            break;
        case CSS_PROP_VISIBILITY:
            style->visibility = parse_visibility(val);
            break;
        case CSS_PROP_OPACITY:
            if (val->type == VAL_NUMBER)
                style->opacity = val->number < 0 ? 0 : (val->number > 1 ? 1 : val->number);
            break;
        case CSS_PROP_TOP:    style->top = *val; break;
        case CSS_PROP_RIGHT:  style->right = *val; break;
        case CSS_PROP_BOTTOM: style->bottom = *val; break;
        case CSS_PROP_LEFT:   style->left = *val; break;
        case CSS_PROP_FLEX_DIRECTION:
            style->flex_direction = parse_flex_direction(val);
            break;
        case CSS_PROP_FLEX_WRAP:
            style->flex_wrap = parse_flex_wrap(val);
            break;
        case CSS_PROP_FLEX_GROW:
            if (val->type == VAL_NUMBER) style->flex_grow = val->number;
            break;
        case CSS_PROP_FLEX_SHRINK:
            if (val->type == VAL_NUMBER) style->flex_shrink = val->number;
            break;
        case CSS_PROP_FLEX_BASIS:
            style->flex_basis = *val;
            break;
        case CSS_PROP_ALIGN_ITEMS:
            style->align_items = *val;
            break;
        case CSS_PROP_ALIGN_SELF:
            style->align_self = *val;
            break;
        case CSS_PROP_JUSTIFY_CONTENT:
            style->justify_content = *val;
            break;
        case CSS_PROP_Z_INDEX:
            style->z_index = *val;
            break;
        case CSS_PROP_TRANSFORM:
            style->has_transform = (val->type != VAL_NONE);
            break;
        case CSS_PROP_GRID_TEMPLATE_COLUMNS:
            style->grid_template_columns = *val;
            break;
        case CSS_PROP_GRID_TEMPLATE_ROWS:
            style->grid_template_rows = *val;
            break;
        case CSS_PROP_GRID_AUTO_COLUMNS:
            style->grid_auto_columns = *val;
            break;
        case CSS_PROP_GRID_AUTO_ROWS:
            style->grid_auto_rows = *val;
            break;
        case CSS_PROP_GRID_AUTO_FLOW:
            style->grid_auto_flow = parse_grid_auto_flow(val);
            break;
        case CSS_PROP_GRID_COLUMN_START:
            style->grid_column_start = *val;
            break;
        case CSS_PROP_GRID_COLUMN_END:
            style->grid_column_end = *val;
            break;
        case CSS_PROP_GRID_ROW_START:
            style->grid_row_start = *val;
            break;
        case CSS_PROP_GRID_ROW_END:
            style->grid_row_end = *val;
            break;
        case CSS_PROP_ROW_GAP:
            style->row_gap = *val;
            break;
        case CSS_PROP_COLUMN_GAP:
            style->column_gap = *val;
            break;
        case CSS_PROP_JUSTIFY_ITEMS:
            style->justify_items = *val;
            break;
        case CSS_PROP_ALIGN_CONTENT:
            style->align_content = *val;
            break;
        case CSS_PROP_GAP:
            style->row_gap = *val;
            style->column_gap = *val;
            break;
        case CSS_PROP_GRID_COLUMN:
            style->grid_column_start = *val;
            break;
        case CSS_PROP_GRID_ROW:
            style->grid_row_start = *val;
            break;
        case CSS_PROP_WRITING_MODE:
            if (val->type == VAL_KEYWORD && val->string) {
                if (strcmp(val->string, "vertical-rl") == 0)
                    style->writing_mode = WM_VERTICAL_RL;
                else if (strcmp(val->string, "vertical-lr") == 0)
                    style->writing_mode = WM_VERTICAL_LR;
                else
                    style->writing_mode = WM_HORIZONTAL_TB;
            }
            break;
        case CSS_PROP_DIRECTION:
            if (val->type == VAL_KEYWORD && val->string) {
                style->direction = (strcmp(val->string, "rtl") == 0)
                    ? DIR_RTL : DIR_LTR;
            }
            break;
        default:
            /* Store in generic values array. */
            if (prop > CSS_PROP_NONE && prop < CSS_PROP__COUNT)
                style->values[prop] = *val;
            break;
        }
    }

    /* Fixup: floated or absolutely positioned elements are blockified. */
    if (style->float_val != FLOAT_NONE ||
        style->position == POSITION_ABSOLUTE ||
        style->position == POSITION_FIXED) {
        if (style->display == DISPLAY_INLINE ||
            style->display == DISPLAY_INLINE_BLOCK ||
            style->display == DISPLAY_INLINE_FLEX ||
            style->display == DISPLAY_INLINE_GRID ||
            style->display == DISPLAY_INLINE_TABLE) {
            /* Blockify. */
            if (style->display == DISPLAY_INLINE_FLEX) style->display = DISPLAY_FLEX;
            else if (style->display == DISPLAY_INLINE_GRID) style->display = DISPLAY_GRID;
            else if (style->display == DISPLAY_INLINE_TABLE) style->display = DISPLAY_TABLE;
            else style->display = DISPLAY_BLOCK;
        }
    }

    /* Border widths are 0 if style is none. */
    if (style->border_top_style == BORDER_NONE) style->border_width.top = 0;
    if (style->border_right_style == BORDER_NONE) style->border_width.right = 0;
    if (style->border_bottom_style == BORDER_NONE) style->border_width.bottom = 0;
    if (style->border_left_style == BORDER_NONE) style->border_width.left = 0;
}
