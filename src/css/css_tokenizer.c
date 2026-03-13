/*
 * Pane — CSS Tokenizer Implementation
 */

#include "css_tokenizer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void css_tokenizer_init(CssTokenizer *t, const char *input, size_t len)
{
    t->input = input;
    t->input_len = len;
    t->pos = 0;
}

static inline bool at_eof(CssTokenizer *t)
{
    return t->pos >= t->input_len;
}

static inline char peek_ch(CssTokenizer *t)
{
    return at_eof(t) ? '\0' : t->input[t->pos];
}

static inline char peek_at(CssTokenizer *t, size_t offset)
{
    size_t p = t->pos + offset;
    return p < t->input_len ? t->input[p] : '\0';
}

static inline char consume_ch(CssTokenizer *t)
{
    return at_eof(t) ? '\0' : t->input[t->pos++];
}

static bool is_name_start(char c)
{
    return isalpha((unsigned char)c) || c == '_' || (unsigned char)c > 127;
}

static bool is_name_char(char c)
{
    return is_name_start(c) || isdigit((unsigned char)c) || c == '-';
}

static bool starts_ident(CssTokenizer *t)
{
    char c = peek_ch(t);
    if (is_name_start(c)) return true;
    if (c == '-') {
        char next = peek_at(t, 1);
        return is_name_start(next) || next == '-';
    }
    if (c == '\\') return true;
    return false;
}

static bool starts_number(CssTokenizer *t)
{
    char c = peek_ch(t);
    if (isdigit((unsigned char)c)) return true;
    if (c == '.') return isdigit((unsigned char)peek_at(t, 1));
    if (c == '+' || c == '-') {
        char next = peek_at(t, 1);
        if (isdigit((unsigned char)next)) return true;
        if (next == '.' && isdigit((unsigned char)peek_at(t, 2))) return true;
    }
    return false;
}

static void consume_name(CssTokenizer *t, const char **start, size_t *len)
{
    *start = t->input + t->pos;
    size_t begin = t->pos;
    while (!at_eof(t) && is_name_char(peek_ch(t)))
        t->pos++;
    *len = t->pos - begin;
}

static float consume_number(CssTokenizer *t, bool *is_int)
{
    const char *start = t->input + t->pos;
    *is_int = true;

    if (peek_ch(t) == '+' || peek_ch(t) == '-') t->pos++;
    while (!at_eof(t) && isdigit((unsigned char)peek_ch(t))) t->pos++;

    if (!at_eof(t) && peek_ch(t) == '.' &&
        t->pos + 1 < t->input_len && isdigit((unsigned char)peek_at(t, 1))) {
        *is_int = false;
        t->pos++; /* consume '.' */
        while (!at_eof(t) && isdigit((unsigned char)peek_ch(t))) t->pos++;
    }

    /* Scientific notation. */
    if (!at_eof(t) && (peek_ch(t) == 'e' || peek_ch(t) == 'E')) {
        char next = peek_at(t, 1);
        if (isdigit((unsigned char)next) || next == '+' || next == '-') {
            *is_int = false;
            t->pos++; /* consume 'e'/'E' */
            if (peek_ch(t) == '+' || peek_ch(t) == '-') t->pos++;
            while (!at_eof(t) && isdigit((unsigned char)peek_ch(t))) t->pos++;
        }
    }

    return strtof(start, NULL);
}

static void consume_whitespace(CssTokenizer *t)
{
    while (!at_eof(t)) {
        char c = peek_ch(t);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')
            t->pos++;
        else
            break;
    }
}

static void consume_string(CssTokenizer *t, char quote)
{
    t->pos++; /* consume opening quote */
    while (!at_eof(t)) {
        char c = consume_ch(t);
        if (c == quote) return;
        if (c == '\\' && !at_eof(t)) t->pos++; /* skip escaped char */
    }
}

static void skip_comment(CssTokenizer *t)
{
    t->pos += 2; /* skip /​* */
    while (t->pos + 1 < t->input_len) {
        if (t->input[t->pos] == '*' && t->input[t->pos + 1] == '/') {
            t->pos += 2;
            return;
        }
        t->pos++;
    }
    t->pos = t->input_len;
}

bool css_tokenizer_next(CssTokenizer *t, CssToken *out)
{
    memset(out, 0, sizeof(*out));

    /* Skip comments. */
    while (t->pos + 1 < t->input_len &&
           t->input[t->pos] == '/' && t->input[t->pos + 1] == '*')
        skip_comment(t);

    if (at_eof(t)) {
        out->type = CSSTOK_EOF;
        return false;
    }

    out->start = t->input + t->pos;
    char c = peek_ch(t);

    /* Whitespace. */
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
        consume_whitespace(t);
        out->type = CSSTOK_WHITESPACE;
        out->len = (t->input + t->pos) - out->start;
        return true;
    }

    /* Strings. */
    if (c == '"' || c == '\'') {
        out->type = CSSTOK_STRING;
        size_t s = t->pos + 1;
        consume_string(t, c);
        out->start = t->input + s;
        out->len = (t->input + t->pos - 1) - out->start;
        if (out->len > t->input_len) out->len = 0; /* guard */
        return true;
    }

    /* Hash. */
    if (c == '#') {
        t->pos++;
        if (!at_eof(t) && is_name_char(peek_ch(t))) {
            out->type = CSSTOK_HASH;
            out->hash_is_id = is_name_start(peek_ch(t));
            consume_name(t, &out->start, &out->len);
            return true;
        }
        out->type = CSSTOK_DELIM;
        out->delim = '#';
        out->len = 1;
        return true;
    }

    /* Number / dimension / percentage. */
    if (starts_number(t)) {
        out->num_value = consume_number(t, &out->num_is_integer);

        if (!at_eof(t) && peek_ch(t) == '%') {
            t->pos++;
            out->type = CSSTOK_PERCENTAGE;
        } else if (!at_eof(t) && starts_ident(t)) {
            out->type = CSSTOK_DIMENSION;
            consume_name(t, &out->unit_start, &out->unit_len);
        } else {
            out->type = CSSTOK_NUMBER;
        }
        out->len = (t->input + t->pos) - out->start;
        return true;
    }

    /* Ident or function. */
    if (starts_ident(t)) {
        const char *name;
        size_t nlen;
        consume_name(t, &name, &nlen);
        out->start = name;
        out->len = nlen;

        if (!at_eof(t) && peek_ch(t) == '(') {
            t->pos++;
            out->type = CSSTOK_FUNCTION;
        } else {
            out->type = CSSTOK_IDENT;
        }
        return true;
    }

    /* At-keyword. */
    if (c == '@') {
        t->pos++;
        if (!at_eof(t) && starts_ident(t)) {
            out->type = CSSTOK_AT_KEYWORD;
            consume_name(t, &out->start, &out->len);
            return true;
        }
        out->type = CSSTOK_DELIM;
        out->delim = '@';
        out->len = 1;
        return true;
    }

    /* Single-character tokens. */
    t->pos++;
    switch (c) {
    case ':': out->type = CSSTOK_COLON; break;
    case ';': out->type = CSSTOK_SEMICOLON; break;
    case ',': out->type = CSSTOK_COMMA; break;
    case '{': out->type = CSSTOK_LBRACE; break;
    case '}': out->type = CSSTOK_RBRACE; break;
    case '(': out->type = CSSTOK_LPAREN; break;
    case ')': out->type = CSSTOK_RPAREN; break;
    case '[': out->type = CSSTOK_LBRACKET; break;
    case ']': out->type = CSSTOK_RBRACKET; break;
    default:
        out->type = CSSTOK_DELIM;
        out->delim = c;
        break;
    }
    out->start = t->input + t->pos - 1;
    out->len = 1;
    return true;
}

void css_tokenizer_peek(CssTokenizer *t, CssToken *out)
{
    size_t saved_pos = t->pos;
    css_tokenizer_next(t, out);
    t->pos = saved_pos;
}
