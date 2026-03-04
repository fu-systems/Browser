/*
 * Pane — HTML Tokenizer Implementation
 *
 * Implements the HTML5 tokenization algorithm as a state machine.
 * Core states for data, tags, attributes, comments, and DOCTYPE.
 */

#include "tokenizer.h"
#include <ctype.h>
#include <string.h>

void tokenizer_init(HtmlTokenizer *t, const char *input, size_t len)
{
    memset(t, 0, sizeof(*t));
    t->input = input;
    t->input_len = len;
    t->state = STATE_DATA;
}

void tokenizer_set_state(HtmlTokenizer *t, TokenizerState state)
{
    t->state = state;
}

static inline bool at_eof(HtmlTokenizer *t)
{
    return t->pos >= t->input_len;
}

static inline char peek(HtmlTokenizer *t)
{
    return at_eof(t) ? '\0' : t->input[t->pos];
}

static inline char consume(HtmlTokenizer *t)
{
    return at_eof(t) ? '\0' : t->input[t->pos++];
}

static inline void reconsume(HtmlTokenizer *t)
{
    if (t->pos > 0) t->pos--;
}

static void emit_char_token(HtmlToken *out, char c)
{
    out->type = TOK_CHARACTER;
    out->ch_data[0] = c;
    out->ch_len = 1;
}

static void emit_eof(HtmlToken *out)
{
    out->type = TOK_EOF;
}

static void init_tag_token(HtmlToken *out, HtmlTokenType type)
{
    memset(out, 0, sizeof(*out));
    out->type = type;
}

static void tag_append_name(HtmlToken *out, char c)
{
    if (out->tag_name_len < sizeof(out->tag_name) - 1)
        out->tag_name[out->tag_name_len++] = (char)tolower((unsigned char)c);
}

static void add_attr_to_token(HtmlToken *out, const char *name, size_t nlen,
                              const char *value, size_t vlen)
{
    if (out->attr_count >= MAX_ATTRS) return;
    TokenAttr *a = &out->attrs[out->attr_count++];
    a->name = name;
    a->name_len = nlen;
    a->value = value;
    a->value_len = vlen;
}

static bool match_str(HtmlTokenizer *t, const char *s, size_t len)
{
    if (t->pos + len > t->input_len) return false;
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char)t->input[t->pos + i]) !=
            tolower((unsigned char)s[i]))
            return false;
    }
    return true;
}

bool tokenizer_next(HtmlTokenizer *t, HtmlToken *out)
{
    memset(out, 0, sizeof(*out));

    for (;;) {
        if (at_eof(t) && t->state == STATE_DATA) {
            emit_eof(out);
            return false;
        }

        switch (t->state) {

        case STATE_DATA: {
            if (at_eof(t)) { emit_eof(out); return false; }
            char c = consume(t);
            if (c == '<') {
                t->state = STATE_TAG_OPEN;
                continue;
            }
            emit_char_token(out, c);
            return true;
        }

        case STATE_RCDATA: {
            if (at_eof(t)) { emit_eof(out); return false; }
            char c = consume(t);
            if (c == '<') { t->state = STATE_RCDATA_LESS_THAN; continue; }
            emit_char_token(out, c);
            return true;
        }

        case STATE_RAWTEXT: {
            if (at_eof(t)) { emit_eof(out); return false; }
            char c = consume(t);
            if (c == '<') { t->state = STATE_RAWTEXT_LESS_THAN; continue; }
            emit_char_token(out, c);
            return true;
        }

        case STATE_SCRIPT_DATA: {
            if (at_eof(t)) { emit_eof(out); return false; }
            char c = consume(t);
            if (c == '<') { t->state = STATE_SCRIPT_DATA_LESS_THAN; continue; }
            emit_char_token(out, c);
            return true;
        }

        case STATE_PLAINTEXT: {
            if (at_eof(t)) { emit_eof(out); return false; }
            emit_char_token(out, consume(t));
            return true;
        }

        case STATE_TAG_OPEN: {
            if (at_eof(t)) {
                emit_char_token(out, '<');
                t->state = STATE_DATA;
                return true;
            }
            char c = peek(t);
            if (c == '!') {
                consume(t);
                t->state = STATE_MARKUP_DECLARATION_OPEN;
                continue;
            }
            if (c == '/') {
                consume(t);
                t->state = STATE_END_TAG_OPEN;
                continue;
            }
            if (isalpha((unsigned char)c)) {
                init_tag_token(out, TOK_START_TAG);
                t->state = STATE_TAG_NAME;
                continue;
            }
            if (c == '?') {
                t->state = STATE_BOGUS_COMMENT;
                out->type = TOK_COMMENT;
                continue;
            }
            emit_char_token(out, '<');
            t->state = STATE_DATA;
            return true;
        }

        case STATE_END_TAG_OPEN: {
            if (at_eof(t)) {
                /* Emit '</' as characters. */
                emit_char_token(out, '<');
                t->state = STATE_DATA;
                t->pos--;
                return true;
            }
            char c = peek(t);
            if (isalpha((unsigned char)c)) {
                init_tag_token(out, TOK_END_TAG);
                t->state = STATE_TAG_NAME;
                continue;
            }
            if (c == '>') {
                consume(t);
                t->state = STATE_DATA;
                continue;
            }
            t->state = STATE_BOGUS_COMMENT;
            out->type = TOK_COMMENT;
            continue;
        }

        case STATE_TAG_NAME: {
            if (at_eof(t)) {
                t->state = STATE_DATA;
                return true;
            }
            char c = consume(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                t->state = STATE_BEFORE_ATTR_NAME;
                continue;
            }
            if (c == '/') {
                t->state = STATE_SELF_CLOSING_START_TAG;
                continue;
            }
            if (c == '>') {
                t->state = STATE_DATA;
                return true;
            }
            tag_append_name(out, c);
            continue;
        }

        case STATE_BEFORE_ATTR_NAME: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                continue;
            }
            if (c == '/' || c == '>') {
                t->state = STATE_AFTER_ATTR_NAME;
                continue;
            }
            if (c == '=') {
                /* Parse error, start new attr with '=' as name char. */
                t->attr_name_len = 0;
                t->attr_value_len = 0;
                t->state = STATE_ATTR_NAME;
                continue;
            }
            t->attr_name_len = 0;
            t->attr_value_len = 0;
            t->state = STATE_ATTR_NAME;
            continue;
        }

        case STATE_ATTR_NAME: {
            if (at_eof(t)) {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_DATA;
                return true;
            }
            char c = consume(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                t->state = STATE_AFTER_ATTR_NAME;
                continue;
            }
            if (c == '/') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_SELF_CLOSING_START_TAG;
                continue;
            }
            if (c == '=') {
                t->state = STATE_BEFORE_ATTR_VALUE;
                continue;
            }
            if (c == '>') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_DATA;
                return true;
            }
            if (t->attr_name_len < sizeof(t->attr_name_buf) - 1)
                t->attr_name_buf[t->attr_name_len++] =
                    (char)tolower((unsigned char)c);
            continue;
        }

        case STATE_AFTER_ATTR_NAME: {
            if (at_eof(t)) {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_DATA;
                return true;
            }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                continue;
            }
            if (c == '/') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                consume(t);
                t->state = STATE_SELF_CLOSING_START_TAG;
                continue;
            }
            if (c == '=') {
                consume(t);
                t->state = STATE_BEFORE_ATTR_VALUE;
                continue;
            }
            if (c == '>') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                consume(t);
                t->state = STATE_DATA;
                return true;
            }
            /* Start a new attribute. */
            add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                              t->attr_value_buf, t->attr_value_len);
            t->attr_name_len = 0;
            t->attr_value_len = 0;
            t->state = STATE_ATTR_NAME;
            continue;
        }

        case STATE_BEFORE_ATTR_VALUE: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                continue;
            }
            if (c == '"') {
                consume(t);
                t->state = STATE_ATTR_VALUE_DOUBLE;
                continue;
            }
            if (c == '\'') {
                consume(t);
                t->state = STATE_ATTR_VALUE_SINGLE;
                continue;
            }
            if (c == '>') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                consume(t);
                t->state = STATE_DATA;
                return true;
            }
            t->state = STATE_ATTR_VALUE_UNQUOTED;
            continue;
        }

        case STATE_ATTR_VALUE_DOUBLE: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '"') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_AFTER_ATTR_VALUE_QUOTED;
                continue;
            }
            if (t->attr_value_len < sizeof(t->attr_value_buf) - 1)
                t->attr_value_buf[t->attr_value_len++] = c;
            continue;
        }

        case STATE_ATTR_VALUE_SINGLE: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '\'') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_AFTER_ATTR_VALUE_QUOTED;
                continue;
            }
            if (t->attr_value_len < sizeof(t->attr_value_buf) - 1)
                t->attr_value_buf[t->attr_value_len++] = c;
            continue;
        }

        case STATE_ATTR_VALUE_UNQUOTED: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_BEFORE_ATTR_NAME;
                continue;
            }
            if (c == '>') {
                add_attr_to_token(out, t->attr_name_buf, t->attr_name_len,
                                  t->attr_value_buf, t->attr_value_len);
                t->state = STATE_DATA;
                return true;
            }
            if (t->attr_value_len < sizeof(t->attr_value_buf) - 1)
                t->attr_value_buf[t->attr_value_len++] = c;
            continue;
        }

        case STATE_AFTER_ATTR_VALUE_QUOTED: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                t->state = STATE_BEFORE_ATTR_NAME;
                continue;
            }
            if (c == '/') {
                consume(t);
                t->state = STATE_SELF_CLOSING_START_TAG;
                continue;
            }
            if (c == '>') {
                consume(t);
                t->state = STATE_DATA;
                return true;
            }
            /* Missing whitespace between attributes. */
            t->state = STATE_BEFORE_ATTR_NAME;
            continue;
        }

        case STATE_SELF_CLOSING_START_TAG: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '>') {
                out->self_closing = true;
                t->state = STATE_DATA;
                return true;
            }
            reconsume(t);
            t->state = STATE_BEFORE_ATTR_NAME;
            continue;
        }

        case STATE_BOGUS_COMMENT: {
            out->type = TOK_COMMENT;
            out->comment_len = 0;
            while (!at_eof(t)) {
                char c = consume(t);
                if (c == '>') break;
                if (out->comment_len < sizeof(out->comment_data) - 1)
                    out->comment_data[out->comment_len++] = c;
            }
            t->state = STATE_DATA;
            return true;
        }

        case STATE_MARKUP_DECLARATION_OPEN: {
            if (match_str(t, "--", 2)) {
                t->pos += 2;
                out->type = TOK_COMMENT;
                out->comment_len = 0;
                t->state = STATE_COMMENT_START;
                continue;
            }
            if (match_str(t, "doctype", 7)) {
                t->pos += 7;
                t->state = STATE_DOCTYPE;
                continue;
            }
            /* Treat as bogus comment. */
            t->state = STATE_BOGUS_COMMENT;
            out->type = TOK_COMMENT;
            out->comment_len = 0;
            continue;
        }

        case STATE_COMMENT_START: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '-') { t->state = STATE_COMMENT_START_DASH; continue; }
            if (c == '>') { t->state = STATE_DATA; return true; }
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = c;
            t->state = STATE_COMMENT;
            continue;
        }

        case STATE_COMMENT_START_DASH: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '-') { t->state = STATE_COMMENT_END; continue; }
            if (c == '>') { t->state = STATE_DATA; return true; }
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = '-';
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = c;
            t->state = STATE_COMMENT;
            continue;
        }

        case STATE_COMMENT: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '-') { t->state = STATE_COMMENT_END_DASH; continue; }
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = c;
            continue;
        }

        case STATE_COMMENT_END_DASH: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '-') { t->state = STATE_COMMENT_END; continue; }
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = '-';
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = c;
            t->state = STATE_COMMENT;
            continue;
        }

        case STATE_COMMENT_END: {
            if (at_eof(t)) { t->state = STATE_DATA; return true; }
            char c = consume(t);
            if (c == '>') { t->state = STATE_DATA; return true; }
            if (c == '-') {
                if (out->comment_len < sizeof(out->comment_data) - 1)
                    out->comment_data[out->comment_len++] = '-';
                continue;
            }
            if (out->comment_len < sizeof(out->comment_data) - 2) {
                out->comment_data[out->comment_len++] = '-';
                out->comment_data[out->comment_len++] = '-';
            }
            if (out->comment_len < sizeof(out->comment_data) - 1)
                out->comment_data[out->comment_len++] = c;
            t->state = STATE_COMMENT;
            continue;
        }

        case STATE_DOCTYPE: {
            if (at_eof(t)) {
                out->type = TOK_DOCTYPE;
                out->force_quirks = true;
                t->state = STATE_DATA;
                return true;
            }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                t->state = STATE_BEFORE_DOCTYPE_NAME;
                continue;
            }
            t->state = STATE_BEFORE_DOCTYPE_NAME;
            continue;
        }

        case STATE_BEFORE_DOCTYPE_NAME: {
            if (at_eof(t)) {
                out->type = TOK_DOCTYPE;
                out->force_quirks = true;
                t->state = STATE_DATA;
                return true;
            }
            char c = peek(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                consume(t);
                continue;
            }
            if (c == '>') {
                consume(t);
                out->type = TOK_DOCTYPE;
                out->force_quirks = true;
                t->state = STATE_DATA;
                return true;
            }
            out->type = TOK_DOCTYPE;
            out->doctype_name_len = 0;
            t->state = STATE_DOCTYPE_NAME;
            continue;
        }

        case STATE_DOCTYPE_NAME: {
            if (at_eof(t)) {
                out->force_quirks = true;
                t->state = STATE_DATA;
                return true;
            }
            char c = consume(t);
            if (c == '\t' || c == '\n' || c == '\f' || c == ' ') {
                t->state = STATE_AFTER_DOCTYPE_NAME;
                continue;
            }
            if (c == '>') {
                t->state = STATE_DATA;
                return true;
            }
            if (out->doctype_name_len < sizeof(out->doctype_name) - 1)
                out->doctype_name[out->doctype_name_len++] =
                    (char)tolower((unsigned char)c);
            continue;
        }

        case STATE_AFTER_DOCTYPE_NAME: {
            /* Simplified: skip to '>'. */
            while (!at_eof(t) && consume(t) != '>')
                ;
            t->state = STATE_DATA;
            return true;
        }

        case STATE_RCDATA_LESS_THAN: {
            if (at_eof(t) || peek(t) != '/') {
                emit_char_token(out, '<');
                t->state = STATE_RCDATA;
                return true;
            }
            consume(t);
            t->temp_len = 0;
            t->state = STATE_RCDATA_END_TAG_OPEN;
            continue;
        }

        case STATE_RCDATA_END_TAG_OPEN: {
            if (at_eof(t) || !isalpha((unsigned char)peek(t))) {
                emit_char_token(out, '<');
                t->state = STATE_RCDATA;
                /* Need to re-emit '/' too, simplified here. */
                return true;
            }
            init_tag_token(out, TOK_END_TAG);
            t->state = STATE_RCDATA_END_TAG_NAME;
            continue;
        }

        case STATE_RCDATA_END_TAG_NAME: {
            if (at_eof(t)) {
                t->state = STATE_RCDATA;
                continue;
            }
            char c = consume(t);
            if ((c == '\t' || c == '\n' || c == '\f' || c == ' ') &&
                out->tag_name_len > 0) {
                t->state = STATE_BEFORE_ATTR_NAME;
                continue;
            }
            if (c == '/' && out->tag_name_len > 0) {
                t->state = STATE_SELF_CLOSING_START_TAG;
                continue;
            }
            if (c == '>' && out->tag_name_len > 0) {
                t->state = STATE_DATA;
                return true;
            }
            if (isalpha((unsigned char)c)) {
                tag_append_name(out, c);
                if (t->temp_len < sizeof(t->temp_buf) - 1)
                    t->temp_buf[t->temp_len++] = c;
                continue;
            }
            /* Not a valid end tag — emit as characters. */
            t->state = STATE_RCDATA;
            continue;
        }

        case STATE_RAWTEXT_LESS_THAN: {
            if (at_eof(t) || peek(t) != '/') {
                emit_char_token(out, '<');
                t->state = STATE_RAWTEXT;
                return true;
            }
            consume(t);
            t->temp_len = 0;
            t->state = STATE_RAWTEXT_END_TAG_OPEN;
            continue;
        }

        case STATE_RAWTEXT_END_TAG_OPEN: {
            if (at_eof(t) || !isalpha((unsigned char)peek(t))) {
                emit_char_token(out, '<');
                t->state = STATE_RAWTEXT;
                return true;
            }
            init_tag_token(out, TOK_END_TAG);
            t->state = STATE_RAWTEXT_END_TAG_NAME;
            continue;
        }

        case STATE_RAWTEXT_END_TAG_NAME: {
            if (at_eof(t)) { t->state = STATE_RAWTEXT; continue; }
            char c = consume(t);
            if (c == '>' && out->tag_name_len > 0) {
                t->state = STATE_DATA;
                return true;
            }
            if (isalpha((unsigned char)c)) {
                tag_append_name(out, c);
                continue;
            }
            t->state = STATE_RAWTEXT;
            continue;
        }

        case STATE_SCRIPT_DATA_LESS_THAN: {
            if (at_eof(t) || peek(t) != '/') {
                emit_char_token(out, '<');
                t->state = STATE_SCRIPT_DATA;
                return true;
            }
            consume(t);
            t->temp_len = 0;
            t->state = STATE_SCRIPT_DATA_END_TAG_OPEN;
            continue;
        }

        case STATE_SCRIPT_DATA_END_TAG_OPEN: {
            if (at_eof(t) || !isalpha((unsigned char)peek(t))) {
                emit_char_token(out, '<');
                t->state = STATE_SCRIPT_DATA;
                return true;
            }
            init_tag_token(out, TOK_END_TAG);
            t->state = STATE_SCRIPT_DATA_END_TAG_NAME;
            continue;
        }

        case STATE_SCRIPT_DATA_END_TAG_NAME: {
            if (at_eof(t)) { t->state = STATE_SCRIPT_DATA; continue; }
            char c = consume(t);
            if (c == '>' && out->tag_name_len > 0) {
                t->state = STATE_DATA;
                return true;
            }
            if (isalpha((unsigned char)c)) {
                tag_append_name(out, c);
                continue;
            }
            t->state = STATE_SCRIPT_DATA;
            continue;
        }

        } /* switch */
    } /* for */
}
