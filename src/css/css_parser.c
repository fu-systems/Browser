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
        CssValue value = css_parse_value(value_start, value_len, arena);

        if (prop_id != CSS_PROP_NONE) {
            CssDeclaration decl = {
                .property = prop_id,
                .value = value,
                .important = important,
            };
            declblock_push(db, decl, arena);
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
