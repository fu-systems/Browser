/*
 * Pane — CSS Parser
 *
 * Parses CSS stylesheets into rule lists: selector + declaration block.
 * Also parses inline styles (declaration blocks without selectors).
 */

#ifndef PANE_CSS_PARSER_H
#define PANE_CSS_PARSER_H

#include "properties.h"
#include "values.h"
#include "selectors.h"
#include "../util/arena.h"

/* ── Declaration ────────────────────────────────────────────────────── */

typedef struct {
    CssPropId  property;
    CssValue   value;
    bool       important;
} CssDeclaration;

/* ── Declaration Block ──────────────────────────────────────────────── */

typedef struct {
    CssDeclaration *decls;
    int             count;
    int             cap;
} DeclBlock;

/* ── Style Rule ─────────────────────────────────────────────────────── */

typedef struct {
    SelectorList  selectors;
    DeclBlock     declarations;
} CssRule;

/* ── Stylesheet ─────────────────────────────────────────────────────── */

typedef struct {
    CssRule *rules;
    int      count;
    int      cap;
    Arena    arena;
} Stylesheet;

/* ── API ────────────────────────────────────────────────────────────── */

/* Parse a full CSS stylesheet. */
Stylesheet *css_parse_stylesheet(const char *input, size_t len);

/* Parse an inline style attribute. */
DeclBlock css_parse_inline_style(const char *input, size_t len, Arena *arena);

/* Free a stylesheet. */
void css_stylesheet_free(Stylesheet *ss);

/* Parse a single CSS value from a token stream. */
CssValue css_parse_value(const char *input, size_t len, Arena *arena);

/* Parse a space-separated list of values (e.g., grid-template-columns: 1fr 1fr 200px). */
CssValue css_parse_value_list(const char *input, size_t len, Arena *arena);

#endif /* PANE_CSS_PARSER_H */
