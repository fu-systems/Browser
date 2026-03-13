/*
 * Pane — CSS Tokenizer
 *
 * Tokenizes CSS input according to the CSS Syntax Module Level 3.
 */

#ifndef PANE_CSS_TOKENIZER_H
#define PANE_CSS_TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    CSSTOK_IDENT,
    CSSTOK_FUNCTION,    /* ident followed by '(' */
    CSSTOK_AT_KEYWORD,  /* @ident */
    CSSTOK_HASH,        /* #ident */
    CSSTOK_STRING,
    CSSTOK_NUMBER,
    CSSTOK_DIMENSION,   /* number + ident */
    CSSTOK_PERCENTAGE,  /* number + % */
    CSSTOK_WHITESPACE,
    CSSTOK_COLON,
    CSSTOK_SEMICOLON,
    CSSTOK_COMMA,
    CSSTOK_LBRACE,
    CSSTOK_RBRACE,
    CSSTOK_LPAREN,
    CSSTOK_RPAREN,
    CSSTOK_LBRACKET,
    CSSTOK_RBRACKET,
    CSSTOK_DELIM,       /* single character delimiter */
    CSSTOK_CDO,         /* <!-- */
    CSSTOK_CDC,         /* --> */
    CSSTOK_EOF,
} CssTokType;

typedef struct {
    CssTokType  type;
    const char *start;
    size_t      len;

    /* For number/dimension/percentage. */
    float       num_value;
    bool        num_is_integer;

    /* For dimension: unit. */
    const char *unit_start;
    size_t      unit_len;

    /* For hash: unrestricted or id type. */
    bool        hash_is_id;

    /* For delim. */
    char        delim;
} CssToken;

typedef struct {
    const char *input;
    size_t      input_len;
    size_t      pos;
} CssTokenizer;

void css_tokenizer_init(CssTokenizer *t, const char *input, size_t len);
bool css_tokenizer_next(CssTokenizer *t, CssToken *out);
void css_tokenizer_peek(CssTokenizer *t, CssToken *out);

#endif /* PANE_CSS_TOKENIZER_H */
