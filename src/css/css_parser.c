/*
 * Pane — CSS Parser Implementation
 *
 * Parses CSS stylesheet text into structured rule lists.
 */

#include "css_parser.h"
#include "css_tokenizer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ── Unit Lookup ────────────────────────────────────────────────────── */

static CssUnit unit_from_name(const char *name, size_t len)
{
    if (len == 2) {
        if (strncmp(name, "px", 2) == 0) return UNIT_PX;
        if (strncmp(name, "em", 2) == 0) return UNIT_EM;
        if (strncmp(name, "vw", 2) == 0) return UNIT_VW;
        if (strncmp(name, "vh", 2) == 0) return UNIT_VH;
        if (strncmp(name, "cm", 2) == 0) return UNIT_CM;
        if (strncmp(name, "mm", 2) == 0) return UNIT_MM;
        if (strncmp(name, "in", 2) == 0) return UNIT_IN;
        if (strncmp(name, "pt", 2) == 0) return UNIT_PT;
        if (strncmp(name, "pc", 2) == 0) return UNIT_PC;
        if (strncmp(name, "ch", 2) == 0) return UNIT_CH;
        if (strncmp(name, "ex", 2) == 0) return UNIT_EX;
        if (strncmp(name, "lh", 2) == 0) return UNIT_LH;
        if (strncmp(name, "fr", 2) == 0) return UNIT_FR;
        if (strncmp(name, "ms", 2) == 0) return UNIT_MS;
    }
    if (len == 3) {
        if (strncmp(name, "rem", 3) == 0) return UNIT_REM;
        if (strncmp(name, "deg", 3) == 0) return UNIT_DEG;
        if (strncmp(name, "rad", 3) == 0) return UNIT_RAD;
    }
    if (len == 4) {
        if (strncmp(name, "vmin", 4) == 0) return UNIT_VMIN;
        if (strncmp(name, "vmax", 4) == 0) return UNIT_VMAX;
    }
    if (len == 1 && name[0] == 's') return UNIT_S;
    return UNIT_PX; /* fallback */
}

/* ── Value Parsing ──────────────────────────────────────────────────── */

static bool str_eq_ci(const char *a, size_t alen, const char *b)
{
    size_t blen = strlen(b);
    if (alen != blen) return false;
    for (size_t i = 0; i < alen; i++) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return false;
    }
    return true;
}

CssValue css_parse_value(const char *input, size_t len, Arena *arena)
{
    CssTokenizer t;
    css_tokenizer_init(&t, input, len);

    CssToken tok;
    /* Skip leading whitespace. */
    do {
        if (!css_tokenizer_next(&t, &tok)) return css_value_initial;
    } while (tok.type == CSSTOK_WHITESPACE);

    switch (tok.type) {
    case CSSTOK_IDENT:
        /* Keywords. */
        if (str_eq_ci(tok.start, tok.len, "auto"))
            return css_value_auto;
        if (str_eq_ci(tok.start, tok.len, "none"))
            return css_value_none;
        if (str_eq_ci(tok.start, tok.len, "initial"))
            return css_value_initial;
        if (str_eq_ci(tok.start, tok.len, "inherit"))
            return css_value_inherit;
        if (str_eq_ci(tok.start, tok.len, "unset"))
            return (CssValue){ .type = VAL_UNSET };
        if (str_eq_ci(tok.start, tok.len, "revert"))
            return (CssValue){ .type = VAL_REVERT };

        /* Try color name. */
        {
            CssColor color;
            if (css_color_from_name(tok.start, tok.len, &color))
                return (CssValue){ .type = VAL_COLOR, .color = color };
        }

        /* Generic keyword. */
        return (CssValue){
            .type = VAL_KEYWORD,
            .string = arena_strndup(arena, tok.start, tok.len),
        };

    case CSSTOK_NUMBER:
        return (CssValue){ .type = VAL_NUMBER, .number = tok.num_value };

    case CSSTOK_DIMENSION:
        return (CssValue){
            .type = VAL_LENGTH,
            .length = {
                .magnitude = tok.num_value,
                .unit = unit_from_name(tok.unit_start, tok.unit_len),
            },
        };

    case CSSTOK_PERCENTAGE:
        return (CssValue){ .type = VAL_PERCENTAGE, .percentage = tok.num_value };

    case CSSTOK_HASH: {
        CssColor color;
        if (css_color_from_hex(tok.start, tok.len, &color))
            return (CssValue){ .type = VAL_COLOR, .color = color };
        return (CssValue){
            .type = VAL_STRING,
            .string = arena_strndup(arena, tok.start, tok.len),
        };
    }

    case CSSTOK_STRING:
        return (CssValue){
            .type = VAL_STRING,
            .string = arena_strndup(arena, tok.start, tok.len),
        };

    case CSSTOK_FUNCTION:
        if (str_eq_ci(tok.start, tok.len, "url")) {
            /* Read URL content until ')'. */
            CssToken content;
            while (css_tokenizer_next(&t, &content)) {
                if (content.type == CSSTOK_RPAREN || content.type == CSSTOK_EOF)
                    break;
                if (content.type == CSSTOK_STRING || content.type == CSSTOK_IDENT) {
                    return (CssValue){
                        .type = VAL_URL,
                        .url = arena_strndup(arena, content.start, content.len),
                    };
                }
            }
            return (CssValue){ .type = VAL_URL, .url = "" };
        }
        /* For rgb(), rgba(), etc. — simplified: store as keyword for now. */
        if (str_eq_ci(tok.start, tok.len, "rgb") ||
            str_eq_ci(tok.start, tok.len, "rgba")) {
            /* Parse rgb(r, g, b) / rgba(r, g, b, a). */
            float vals[4] = {0, 0, 0, 255};
            int vi = 0;
            CssToken arg;
            while (css_tokenizer_next(&t, &arg)) {
                if (arg.type == CSSTOK_RPAREN || arg.type == CSSTOK_EOF) break;
                if ((arg.type == CSSTOK_NUMBER || arg.type == CSSTOK_PERCENTAGE)
                    && vi < 4) {
                    vals[vi++] = arg.num_value;
                }
            }
            return (CssValue){
                .type = VAL_COLOR,
                .color = { (uint8_t)vals[0], (uint8_t)vals[1],
                           (uint8_t)vals[2],
                           vi >= 4 ? (uint8_t)(vals[3] * 255) : 255 },
            };
        }
        /* Skip function content. */
        {
            int depth = 1;
            CssToken ft;
            while (depth > 0 && css_tokenizer_next(&t, &ft)) {
                if (ft.type == CSSTOK_FUNCTION || ft.type == CSSTOK_LPAREN) depth++;
                if (ft.type == CSSTOK_RPAREN) depth--;
                if (ft.type == CSSTOK_EOF) break;
            }
        }
        return (CssValue){
            .type = VAL_KEYWORD,
            .string = arena_strndup(arena, tok.start, tok.len),
        };

    default:
        return css_value_initial;
    }
}

/* ── Value list parsing (space-separated) ─────────────────────────── */

CssValue css_parse_value_list(const char *input, size_t len, Arena *arena)
{
    CssTokenizer t;
    css_tokenizer_init(&t, input, len);

    CssValue items[64];
    int count = 0;

    CssToken tok;
    while (css_tokenizer_next(&t, &tok)) {
        if (tok.type == CSSTOK_EOF) break;
        if (tok.type == CSSTOK_WHITESPACE) continue;
        if (tok.type == CSSTOK_SEMICOLON || tok.type == CSSTOK_RBRACE) break;
        if (count >= 64) break;

        switch (tok.type) {
        case CSSTOK_IDENT:
            if (str_eq_ci(tok.start, tok.len, "auto"))
                items[count++] = css_value_auto;
            else
                items[count++] = (CssValue){
                    .type = VAL_KEYWORD,
                    .string = arena_strndup(arena, tok.start, tok.len),
                };
            break;
        case CSSTOK_NUMBER:
            items[count++] = (CssValue){ .type = VAL_NUMBER, .number = tok.num_value };
            break;
        case CSSTOK_DIMENSION:
            items[count++] = (CssValue){
                .type = VAL_LENGTH,
                .length = {
                    .magnitude = tok.num_value,
                    .unit = unit_from_name(tok.unit_start, tok.unit_len),
                },
            };
            break;
        case CSSTOK_PERCENTAGE:
            items[count++] = (CssValue){ .type = VAL_PERCENTAGE, .percentage = tok.num_value };
            break;
        default:
            break;
        }
    }

    if (count == 0) return css_value_none;
    if (count == 1) return items[0];

    CssValue *list_items = arena_alloc(arena, count * sizeof(CssValue), 8);
    memcpy(list_items, items, count * sizeof(CssValue));
    return (CssValue){
        .type = VAL_LIST,
        .list = { .items = list_items, .count = count },
    };
}

/* Forward declaration. */
static void declblock_push(DeclBlock *db, CssDeclaration decl, Arena *arena);

/* ── Shorthand Expansion ───────────────────────────────────────────── */

/* Parse up to 4 space-separated values for box-side shorthands
 * (margin, padding, border-width, border-style, border-color).
 * Returns the number of values parsed (1-4). */
static int parse_box_values(const char *input, size_t len, Arena *arena,
                            CssValue out[4])
{
    CssTokenizer t;
    css_tokenizer_init(&t, input, len);
    int n = 0;
    CssToken tok;
    while (n < 4 && css_tokenizer_next(&t, &tok)) {
        if (tok.type == CSSTOK_EOF) break;
        if (tok.type == CSSTOK_WHITESPACE) continue;
        if (tok.type == CSSTOK_DELIM && tok.delim == '!') break; /* !important */
        /* Re-parse this single token as a value. */
        out[n++] = css_parse_value(tok.start, tok.len, arena);
    }
    return n;
}

/* Apply CSS 1-4 value box shorthand logic:
 * 1 value:  all sides
 * 2 values: top/bottom, left/right
 * 3 values: top, left/right, bottom
 * 4 values: top, right, bottom, left */
static void box_sides_from_values(const CssValue *vals, int n,
                                  CssValue *top, CssValue *right,
                                  CssValue *bottom, CssValue *left)
{
    switch (n) {
    case 1:
        *top = *right = *bottom = *left = vals[0];
        break;
    case 2:
        *top = *bottom = vals[0];
        *right = *left = vals[1];
        break;
    case 3:
        *top = vals[0];
        *right = *left = vals[1];
        *bottom = vals[2];
        break;
    case 4: default:
        *top = vals[0]; *right = vals[1];
        *bottom = vals[2]; *left = vals[3];
        break;
    }
}

/* Emit 4 longhand declarations for a box-side shorthand.
 * prop_ids: [top, right, bottom, left] */
static void expand_box_shorthand(DeclBlock *db, Arena *arena,
                                 const char *value_start, size_t value_len,
                                 bool important,
                                 CssPropId top_id, CssPropId right_id,
                                 CssPropId bottom_id, CssPropId left_id)
{
    CssValue vals[4];
    int n = parse_box_values(value_start, value_len, arena, vals);
    if (n == 0) return;
    CssValue t, r, b, l;
    box_sides_from_values(vals, n, &t, &r, &b, &l);
    CssDeclaration decls[4] = {
        { .property = top_id,    .value = t, .important = important },
        { .property = right_id,  .value = r, .important = important },
        { .property = bottom_id, .value = b, .important = important },
        { .property = left_id,   .value = l, .important = important },
    };
    for (int i = 0; i < 4; i++)
        declblock_push(db, decls[i], arena);
}

/* Parse compound border shorthand: border: [width] [style] [color]
 * Components can appear in any order. */
static void expand_border_compound(DeclBlock *db, Arena *arena,
                                   const char *value_start, size_t value_len,
                                   bool important)
{
    CssTokenizer t;
    css_tokenizer_init(&t, value_start, value_len);
    CssValue width_val = { .type = VAL_INITIAL };
    CssValue style_val = { .type = VAL_INITIAL };
    CssValue color_val = { .type = VAL_INITIAL };
    bool has_width = false, has_style = false, has_color = false;

    CssToken tok;
    while (css_tokenizer_next(&t, &tok)) {
        if (tok.type == CSSTOK_EOF) break;
        if (tok.type == CSSTOK_WHITESPACE) continue;
        if (tok.type == CSSTOK_DELIM && tok.delim == '!') break;

        if (tok.type == CSSTOK_DIMENSION || tok.type == CSSTOK_NUMBER ||
            tok.type == CSSTOK_PERCENTAGE) {
            /* Width component */
            if (!has_width) {
                width_val = css_parse_value(tok.start, tok.len, arena);
                has_width = true;
            }
        } else if (tok.type == CSSTOK_IDENT) {
            /* Check if it's a border style keyword */
            if (str_eq_ci(tok.start, tok.len, "none") ||
                str_eq_ci(tok.start, tok.len, "solid") ||
                str_eq_ci(tok.start, tok.len, "dashed") ||
                str_eq_ci(tok.start, tok.len, "dotted") ||
                str_eq_ci(tok.start, tok.len, "double") ||
                str_eq_ci(tok.start, tok.len, "groove") ||
                str_eq_ci(tok.start, tok.len, "ridge") ||
                str_eq_ci(tok.start, tok.len, "inset") ||
                str_eq_ci(tok.start, tok.len, "outset") ||
                str_eq_ci(tok.start, tok.len, "hidden")) {
                if (!has_style) {
                    style_val = css_parse_value(tok.start, tok.len, arena);
                    has_style = true;
                }
            } else {
                /* Color keyword or other */
                if (!has_color) {
                    color_val = css_parse_value(tok.start, tok.len, arena);
                    has_color = true;
                }
            }
        } else if (tok.type == CSSTOK_HASH) {
            /* Hex color */
            if (!has_color) {
                /* Need to include the # prefix for parsing */
                color_val = css_parse_value(tok.start - 1, tok.len + 1, arena);
                has_color = true;
            }
        }
    }

    CssPropId width_ids[] = { CSS_PROP_BORDER_TOP_WIDTH, CSS_PROP_BORDER_RIGHT_WIDTH,
                               CSS_PROP_BORDER_BOTTOM_WIDTH, CSS_PROP_BORDER_LEFT_WIDTH };
    CssPropId style_ids[] = { CSS_PROP_BORDER_TOP_STYLE, CSS_PROP_BORDER_RIGHT_STYLE,
                               CSS_PROP_BORDER_BOTTOM_STYLE, CSS_PROP_BORDER_LEFT_STYLE };
    CssPropId color_ids[] = { CSS_PROP_BORDER_TOP_COLOR, CSS_PROP_BORDER_RIGHT_COLOR,
                               CSS_PROP_BORDER_BOTTOM_COLOR, CSS_PROP_BORDER_LEFT_COLOR };
    for (int i = 0; i < 4; i++) {
        if (has_width)
            declblock_push(db, (CssDeclaration){ width_ids[i], width_val, important }, arena);
        if (has_style)
            declblock_push(db, (CssDeclaration){ style_ids[i], style_val, important }, arena);
        if (has_color)
            declblock_push(db, (CssDeclaration){ color_ids[i], color_val, important }, arena);
    }
}

/* Try to expand a shorthand property into longhands.
 * Returns true if expansion was handled (longhands pushed to db). */
static bool try_expand_shorthand(CssPropId prop_id, const char *value_start,
                                 size_t value_len, bool important,
                                 DeclBlock *db, Arena *arena)
{
    switch (prop_id) {
    case CSS_PROP_MARGIN:
        expand_box_shorthand(db, arena, value_start, value_len, important,
            CSS_PROP_MARGIN_TOP, CSS_PROP_MARGIN_RIGHT,
            CSS_PROP_MARGIN_BOTTOM, CSS_PROP_MARGIN_LEFT);
        return true;
    case CSS_PROP_PADDING:
        expand_box_shorthand(db, arena, value_start, value_len, important,
            CSS_PROP_PADDING_TOP, CSS_PROP_PADDING_RIGHT,
            CSS_PROP_PADDING_BOTTOM, CSS_PROP_PADDING_LEFT);
        return true;
    case CSS_PROP_BORDER_WIDTH:
        expand_box_shorthand(db, arena, value_start, value_len, important,
            CSS_PROP_BORDER_TOP_WIDTH, CSS_PROP_BORDER_RIGHT_WIDTH,
            CSS_PROP_BORDER_BOTTOM_WIDTH, CSS_PROP_BORDER_LEFT_WIDTH);
        return true;
    case CSS_PROP_BORDER_STYLE:
        expand_box_shorthand(db, arena, value_start, value_len, important,
            CSS_PROP_BORDER_TOP_STYLE, CSS_PROP_BORDER_RIGHT_STYLE,
            CSS_PROP_BORDER_BOTTOM_STYLE, CSS_PROP_BORDER_LEFT_STYLE);
        return true;
    case CSS_PROP_BORDER_COLOR:
        expand_box_shorthand(db, arena, value_start, value_len, important,
            CSS_PROP_BORDER_TOP_COLOR, CSS_PROP_BORDER_RIGHT_COLOR,
            CSS_PROP_BORDER_BOTTOM_COLOR, CSS_PROP_BORDER_LEFT_COLOR);
        return true;
    case CSS_PROP_BORDER:
        expand_border_compound(db, arena, value_start, value_len, important);
        return true;
    case CSS_PROP_BORDER_TOP:
    case CSS_PROP_BORDER_RIGHT:
    case CSS_PROP_BORDER_BOTTOM:
    case CSS_PROP_BORDER_LEFT: {
        /* border-<side>: [width] [style] [color] — same compound parsing */
        CssPropId w, s, c;
        switch (prop_id) {
        case CSS_PROP_BORDER_TOP:
            w = CSS_PROP_BORDER_TOP_WIDTH; s = CSS_PROP_BORDER_TOP_STYLE;
            c = CSS_PROP_BORDER_TOP_COLOR; break;
        case CSS_PROP_BORDER_RIGHT:
            w = CSS_PROP_BORDER_RIGHT_WIDTH; s = CSS_PROP_BORDER_RIGHT_STYLE;
            c = CSS_PROP_BORDER_RIGHT_COLOR; break;
        case CSS_PROP_BORDER_BOTTOM:
            w = CSS_PROP_BORDER_BOTTOM_WIDTH; s = CSS_PROP_BORDER_BOTTOM_STYLE;
            c = CSS_PROP_BORDER_BOTTOM_COLOR; break;
        default: /* CSS_PROP_BORDER_LEFT */
            w = CSS_PROP_BORDER_LEFT_WIDTH; s = CSS_PROP_BORDER_LEFT_STYLE;
            c = CSS_PROP_BORDER_LEFT_COLOR; break;
        }
        /* Reuse compound parser logic inline */
        CssTokenizer bt;
        css_tokenizer_init(&bt, value_start, value_len);
        CssValue wv = { .type = VAL_INITIAL }, sv = { .type = VAL_INITIAL },
                 cv = { .type = VAL_INITIAL };
        bool hw = false, hs = false, hc = false;
        CssToken btok;
        while (css_tokenizer_next(&bt, &btok)) {
            if (btok.type == CSSTOK_EOF) break;
            if (btok.type == CSSTOK_WHITESPACE) continue;
            if (btok.type == CSSTOK_DELIM && btok.delim == '!') break;
            if (btok.type == CSSTOK_DIMENSION || btok.type == CSSTOK_NUMBER ||
                btok.type == CSSTOK_PERCENTAGE) {
                if (!hw) {
                    wv = css_parse_value(btok.start, btok.len, arena);
                    hw = true;
                }
            } else if (btok.type == CSSTOK_IDENT) {
                if (str_eq_ci(btok.start, btok.len, "none") ||
                    str_eq_ci(btok.start, btok.len, "solid") ||
                    str_eq_ci(btok.start, btok.len, "dashed") ||
                    str_eq_ci(btok.start, btok.len, "dotted") ||
                    str_eq_ci(btok.start, btok.len, "double") ||
                    str_eq_ci(btok.start, btok.len, "groove") ||
                    str_eq_ci(btok.start, btok.len, "ridge") ||
                    str_eq_ci(btok.start, btok.len, "inset") ||
                    str_eq_ci(btok.start, btok.len, "outset") ||
                    str_eq_ci(btok.start, btok.len, "hidden")) {
                    if (!hs) { sv = css_parse_value(btok.start, btok.len, arena); hs = true; }
                } else {
                    if (!hc) { cv = css_parse_value(btok.start, btok.len, arena); hc = true; }
                }
            } else if (btok.type == CSSTOK_HASH) {
                if (!hc) { cv = css_parse_value(btok.start - 1, btok.len + 1, arena); hc = true; }
            }
        }
        if (hw) declblock_push(db, (CssDeclaration){ w, wv, important }, arena);
        if (hs) declblock_push(db, (CssDeclaration){ s, sv, important }, arena);
        if (hc) declblock_push(db, (CssDeclaration){ c, cv, important }, arena);
        return true;
    }
    case CSS_PROP_OVERFLOW: {
        /* overflow: x [y] */
        CssValue vals[4];
        int n = parse_box_values(value_start, value_len, arena, vals);
        if (n >= 1) {
            declblock_push(db, (CssDeclaration){ CSS_PROP_OVERFLOW_X, vals[0], important }, arena);
            declblock_push(db, (CssDeclaration){ CSS_PROP_OVERFLOW_Y,
                n >= 2 ? vals[1] : vals[0], important }, arena);
        }
        return true;
    }
    case CSS_PROP_FLEX: {
        /* flex: [grow] [shrink] [basis] — simplified */
        CssValue vals[4];
        int n = parse_box_values(value_start, value_len, arena, vals);
        if (n == 1) {
            /* flex: <number> means grow=<n>, shrink=1, basis=0 */
            if (vals[0].type == VAL_NUMBER) {
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_GROW, vals[0], important }, arena);
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_SHRINK,
                    (CssValue){ .type = VAL_NUMBER, .number = 1 }, important }, arena);
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_BASIS,
                    (CssValue){ .type = VAL_LENGTH, .length = { 0, UNIT_PX } }, important }, arena);
            } else {
                /* flex: auto, none, or a length (basis) */
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_BASIS, vals[0], important }, arena);
            }
        } else if (n == 2) {
            declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_GROW, vals[0], important }, arena);
            if (vals[1].type == VAL_NUMBER)
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_SHRINK, vals[1], important }, arena);
            else
                declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_BASIS, vals[1], important }, arena);
        } else if (n >= 3) {
            declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_GROW, vals[0], important }, arena);
            declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_SHRINK, vals[1], important }, arena);
            declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_BASIS, vals[2], important }, arena);
        }
        return true;
    }
    case CSS_PROP_FLEX_FLOW: {
        /* flex-flow: [direction] [wrap] */
        CssValue vals[4];
        int n = parse_box_values(value_start, value_len, arena, vals);
        for (int i = 0; i < n; i++) {
            if (vals[i].type == VAL_KEYWORD && vals[i].string) {
                if (str_eq_ci(vals[i].string, strlen(vals[i].string), "wrap") ||
                    str_eq_ci(vals[i].string, strlen(vals[i].string), "nowrap") ||
                    str_eq_ci(vals[i].string, strlen(vals[i].string), "wrap-reverse"))
                    declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_WRAP, vals[i], important }, arena);
                else
                    declblock_push(db, (CssDeclaration){ CSS_PROP_FLEX_DIRECTION, vals[i], important }, arena);
            }
        }
        return true;
    }
    case CSS_PROP_GAP: {
        CssValue vals[4];
        int n = parse_box_values(value_start, value_len, arena, vals);
        if (n >= 1) {
            declblock_push(db, (CssDeclaration){ CSS_PROP_ROW_GAP, vals[0], important }, arena);
            declblock_push(db, (CssDeclaration){ CSS_PROP_COLUMN_GAP,
                n >= 2 ? vals[1] : vals[0], important }, arena);
        }
        return true;
    }
    default:
        return false;
    }
}

/* ── Declaration Parsing ────────────────────────────────────────────── */

static void declblock_push(DeclBlock *db, CssDeclaration decl, Arena *arena)
{
    if (db->count >= db->cap) {
        int new_cap = db->cap ? db->cap * 2 : 8;
        CssDeclaration *new_decls = arena_alloc(arena, new_cap * sizeof(CssDeclaration), 8);
        if (db->count > 0)
            memcpy(new_decls, db->decls, db->count * sizeof(CssDeclaration));
        db->decls = new_decls;
        db->cap = new_cap;
    }
    db->decls[db->count++] = decl;
}

static void parse_declarations(CssTokenizer *t, DeclBlock *db, Arena *arena)
{
    while (true) {
        CssToken tok;
        if (!css_tokenizer_next(t, &tok)) return;
        if (tok.type == CSSTOK_EOF || tok.type == CSSTOK_RBRACE) return;
        if (tok.type == CSSTOK_WHITESPACE || tok.type == CSSTOK_SEMICOLON)
            continue;

        /* Property name. */
        if (tok.type != CSSTOK_IDENT) {
            /* Skip to next semicolon or brace. */
            while (css_tokenizer_next(t, &tok)) {
                if (tok.type == CSSTOK_SEMICOLON || tok.type == CSSTOK_RBRACE ||
                    tok.type == CSSTOK_EOF) break;
            }
            if (tok.type == CSSTOK_RBRACE || tok.type == CSSTOK_EOF) return;
            continue;
        }

        /* Lowercase property name. */
        char prop_name[128];
        size_t pnlen = tok.len < sizeof(prop_name) - 1 ? tok.len : sizeof(prop_name) - 1;
        memcpy(prop_name, tok.start, pnlen);
        prop_name[pnlen] = '\0';
        str_ascii_lower(prop_name, pnlen);

        CssPropId prop_id = css_prop_from_name(prop_name, pnlen);

        /* Skip whitespace + colon. */
        do { css_tokenizer_next(t, &tok); } while (tok.type == CSSTOK_WHITESPACE);
        if (tok.type != CSSTOK_COLON) {
            /* Skip to semicolon. */
            while (css_tokenizer_next(t, &tok)) {
                if (tok.type == CSSTOK_SEMICOLON || tok.type == CSSTOK_RBRACE ||
                    tok.type == CSSTOK_EOF) break;
            }
            if (tok.type == CSSTOK_RBRACE || tok.type == CSSTOK_EOF) return;
            continue;
        }

        /* Collect value tokens until ; or }. */
        const char *value_start = t->input + t->pos;
        size_t value_end_pos = t->pos;
        bool important = false;

        while (css_tokenizer_next(t, &tok)) {
            if (tok.type == CSSTOK_SEMICOLON || tok.type == CSSTOK_RBRACE ||
                tok.type == CSSTOK_EOF) break;
            if (tok.type == CSSTOK_DELIM && tok.delim == '!') {
                CssToken bang;
                css_tokenizer_next(t, &bang);
                if (bang.type == CSSTOK_IDENT &&
                    str_eq_ci(bang.start, bang.len, "important"))
                    important = true;
                continue;
            }
            value_end_pos = t->pos;
        }

        size_t value_len = value_end_pos - (value_start - t->input);

        if (prop_id != CSS_PROP_NONE) {
            /* Try shorthand expansion first. */
            if (!try_expand_shorthand(prop_id, value_start, value_len,
                                      important, db, arena)) {
                /* Not a shorthand — parse as single value. */
                CssValue value;
                if (prop_id == CSS_PROP_GRID_TEMPLATE_COLUMNS ||
                    prop_id == CSS_PROP_GRID_TEMPLATE_ROWS) {
                    value = css_parse_value_list(value_start, value_len, arena);
                } else {
                    value = css_parse_value(value_start, value_len, arena);
                }
                CssDeclaration decl = {
                    .property = prop_id,
                    .value = value,
                    .important = important,
                };
                declblock_push(db, decl, arena);
            }
        }

        if (tok.type == CSSTOK_RBRACE || tok.type == CSSTOK_EOF) return;
    }
}

/* ── Stylesheet Parsing ─────────────────────────────────────────────── */

static void rule_push(Stylesheet *ss, CssRule rule)
{
    if (ss->count >= ss->cap) {
        int new_cap = ss->cap ? ss->cap * 2 : 16;
        CssRule *new_rules = arena_alloc(&ss->arena, new_cap * sizeof(CssRule), 8);
        if (ss->count > 0)
            memcpy(new_rules, ss->rules, ss->count * sizeof(CssRule));
        ss->rules = new_rules;
        ss->cap = new_cap;
    }
    ss->rules[ss->count++] = rule;
}

Stylesheet *css_parse_stylesheet(const char *input, size_t len)
{
    Stylesheet *ss = calloc(1, sizeof(Stylesheet));
    arena_init(&ss->arena, 0);
    ss->cap = 16;
    ss->rules = arena_alloc(&ss->arena, ss->cap * sizeof(CssRule), 8);

    CssTokenizer t;
    css_tokenizer_init(&t, input, len);

    while (true) {
        CssToken tok;
        if (!css_tokenizer_next(&t, &tok)) break;
        if (tok.type == CSSTOK_EOF) break;
        if (tok.type == CSSTOK_WHITESPACE || tok.type == CSSTOK_CDO ||
            tok.type == CSSTOK_CDC) continue;

        /* At-rules: skip for now. */
        if (tok.type == CSSTOK_AT_KEYWORD) {
            int depth = 0;
            while (css_tokenizer_next(&t, &tok)) {
                if (tok.type == CSSTOK_LBRACE) depth++;
                if (tok.type == CSSTOK_RBRACE) {
                    if (--depth <= 0) break;
                }
                if (tok.type == CSSTOK_SEMICOLON && depth == 0) break;
                if (tok.type == CSSTOK_EOF) break;
            }
            continue;
        }

        /* Style rule: collect selector, then parse declarations. */
        const char *sel_start = tok.start;
        size_t sel_end_pos = t.pos;

        while (tok.type != CSSTOK_LBRACE && tok.type != CSSTOK_EOF) {
            sel_end_pos = t.pos;
            if (!css_tokenizer_next(&t, &tok)) break;
        }

        if (tok.type != CSSTOK_LBRACE) break;

        size_t sel_len = sel_end_pos - (sel_start - t.input);

        CssRule rule;
        memset(&rule, 0, sizeof(rule));

        selector_parse(sel_start, sel_len, &ss->arena, &rule.selectors);

        parse_declarations(&t, &rule.declarations, &ss->arena);

        if (rule.selectors.count > 0 && rule.declarations.count > 0)
            rule_push(ss, rule);
    }

    return ss;
}

DeclBlock css_parse_inline_style(const char *input, size_t len, Arena *arena)
{
    DeclBlock db = {0};
    CssTokenizer t;
    css_tokenizer_init(&t, input, len);
    parse_declarations(&t, &db, arena);
    return db;
}

void css_stylesheet_free(Stylesheet *ss)
{
    if (!ss) return;
    arena_destroy(&ss->arena);
    free(ss);
}
