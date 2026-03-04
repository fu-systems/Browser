/*
 * Pane — HTML Tokenizer
 *
 * State machine tokenizer following the HTML5 specification.
 * Emits tokens consumed by the tree builder.
 */

#ifndef PANE_HTML_TOKENIZER_H
#define PANE_HTML_TOKENIZER_H

#include "../util/arena.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Token Types ────────────────────────────────────────────────────── */

typedef enum {
    TOK_DOCTYPE,
    TOK_START_TAG,
    TOK_END_TAG,
    TOK_COMMENT,
    TOK_CHARACTER,
    TOK_EOF,
} HtmlTokenType;

#define MAX_ATTRS 32
#define TOKEN_BUF_SIZE 4096

typedef struct {
    const char *name;
    size_t      name_len;
    const char *value;
    size_t      value_len;
} TokenAttr;

typedef struct {
    HtmlTokenType type;

    /* For START_TAG / END_TAG. */
    char     tag_name[128];
    size_t   tag_name_len;
    bool     self_closing;
    TokenAttr attrs[MAX_ATTRS];
    uint16_t  attr_count;

    /* For CHARACTER. */
    char     ch_data[TOKEN_BUF_SIZE];
    size_t   ch_len;

    /* For COMMENT. */
    char     comment_data[TOKEN_BUF_SIZE];
    size_t   comment_len;

    /* For DOCTYPE. */
    char     doctype_name[128];
    size_t   doctype_name_len;
    bool     force_quirks;
} HtmlToken;

/* ── Tokenizer States ───────────────────────────────────────────────── */

typedef enum {
    STATE_DATA,
    STATE_RCDATA,
    STATE_RAWTEXT,
    STATE_SCRIPT_DATA,
    STATE_PLAINTEXT,
    STATE_TAG_OPEN,
    STATE_END_TAG_OPEN,
    STATE_TAG_NAME,
    STATE_RCDATA_LESS_THAN,
    STATE_RCDATA_END_TAG_OPEN,
    STATE_RCDATA_END_TAG_NAME,
    STATE_RAWTEXT_LESS_THAN,
    STATE_RAWTEXT_END_TAG_OPEN,
    STATE_RAWTEXT_END_TAG_NAME,
    STATE_SCRIPT_DATA_LESS_THAN,
    STATE_SCRIPT_DATA_END_TAG_OPEN,
    STATE_SCRIPT_DATA_END_TAG_NAME,
    STATE_BEFORE_ATTR_NAME,
    STATE_ATTR_NAME,
    STATE_AFTER_ATTR_NAME,
    STATE_BEFORE_ATTR_VALUE,
    STATE_ATTR_VALUE_DOUBLE,
    STATE_ATTR_VALUE_SINGLE,
    STATE_ATTR_VALUE_UNQUOTED,
    STATE_AFTER_ATTR_VALUE_QUOTED,
    STATE_SELF_CLOSING_START_TAG,
    STATE_BOGUS_COMMENT,
    STATE_MARKUP_DECLARATION_OPEN,
    STATE_COMMENT_START,
    STATE_COMMENT_START_DASH,
    STATE_COMMENT,
    STATE_COMMENT_END_DASH,
    STATE_COMMENT_END,
    STATE_DOCTYPE,
    STATE_BEFORE_DOCTYPE_NAME,
    STATE_DOCTYPE_NAME,
    STATE_AFTER_DOCTYPE_NAME,
} TokenizerState;

/* ── Tokenizer ──────────────────────────────────────────────────────── */

typedef struct {
    const char    *input;
    size_t         input_len;
    size_t         pos;
    TokenizerState state;
    TokenizerState return_state;

    /* Temporary tag name for end tag matching in RCDATA/RAWTEXT/script. */
    char    temp_buf[128];
    size_t  temp_len;

    /* Current attribute being built. */
    char    attr_name_buf[256];
    size_t  attr_name_len;
    char    attr_value_buf[4096];
    size_t  attr_value_len;
} HtmlTokenizer;

/* Initialize a tokenizer for the given input. */
void tokenizer_init(HtmlTokenizer *t, const char *input, size_t len);

/* Get the next token. Returns false at EOF. */
bool tokenizer_next(HtmlTokenizer *t, HtmlToken *out);

/* Switch tokenizer state (called by tree builder for <script>, <textarea>, etc). */
void tokenizer_set_state(HtmlTokenizer *t, TokenizerState state);

#endif /* PANE_HTML_TOKENIZER_H */
