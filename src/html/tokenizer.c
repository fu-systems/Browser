/*
 * Pane — HTML Tokenizer Implementation
 *
 * Implements the HTML5 tokenization algorithm as a state machine.
 * Core states for data, tags, attributes, comments, and DOCTYPE.
 */

#include "tokenizer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* ── HTML Named Character References ──────────────────────────────── */

typedef struct { const char *name; const char *value; } NamedEntity;

static const NamedEntity entities[] = {
    {"amp",      "&"},    {"lt",       "<"},    {"gt",       ">"},
    {"quot",     "\""},   {"apos",     "'"},    {"nbsp",     "\xc2\xa0"},
    {"copy",     "\xc2\xa9"}, {"reg",  "\xc2\xae"}, {"trade","\xe2\x84\xa2"},
    {"laquo",    "\xc2\xab"}, {"raquo","\xc2\xbb"},
    {"mdash",    "\xe2\x80\x94"}, {"ndash","\xe2\x80\x93"},
    {"lsquo",    "\xe2\x80\x98"}, {"rsquo","\xe2\x80\x99"},
    {"ldquo",    "\xe2\x80\x9c"}, {"rdquo","\xe2\x80\x9d"},
    {"bull",     "\xe2\x80\xa2"}, {"hellip","\xe2\x80\xa6"},
    {"prime",    "\xe2\x80\xb2"}, {"Prime","\xe2\x80\xb3"},
    {"times",    "\xc3\x97"}, {"divide","\xc3\xb7"},
    {"minus",    "\xe2\x88\x92"}, {"plusmn","\xc2\xb1"},
    {"deg",      "\xc2\xb0"}, {"micro","\xc2\xb5"},
    {"para",     "\xc2\xb6"}, {"middot","\xc2\xb7"},
    {"cent",     "\xc2\xa2"}, {"pound","\xc2\xa3"},
    {"euro",     "\xe2\x82\xac"}, {"yen","\xc2\xa5"},
    {"sect",     "\xc2\xa7"}, {"iexcl","\xc2\xa1"},
    {"iquest",   "\xc2\xbf"}, {"ordf","\xc2\xaa"},
    {"ordm",     "\xc2\xba"}, {"not","\xc2\xac"},
    {"shy",      "\xc2\xad"}, {"macr","\xc2\xaf"},
    {"acute",    "\xc2\xb4"}, {"cedil","\xc2\xb8"},
    {"sup1",     "\xc2\xb9"}, {"sup2","\xc2\xb2"},
    {"sup3",     "\xc2\xb3"}, {"frac14","\xc2\xbc"},
    {"frac12",   "\xc2\xbd"}, {"frac34","\xc2\xbe"},
    {"larr",     "\xe2\x86\x90"}, {"uarr","\xe2\x86\x91"},
    {"rarr",     "\xe2\x86\x92"}, {"darr","\xe2\x86\x93"},
    {"hearts",   "\xe2\x99\xa5"}, {"diams","\xe2\x99\xa6"},
    {"clubs",    "\xe2\x99\xa3"}, {"spades","\xe2\x99\xa0"},
    {"loz",      "\xe2\x97\x8a"}, {"crarr","\xe2\x86\xb5"},
    {"ensp",     "\xe2\x80\x82"}, {"emsp","\xe2\x80\x83"},
    {"thinsp",   "\xe2\x80\x89"}, {"zwnj","\xe2\x80\x8c"},
    {"zwj",      "\xe2\x80\x8d"}, {"lrm","\xe2\x80\x8e"},
    {"rlm",      "\xe2\x80\x8f"},
    /* Greek letters (subset) */
    {"alpha",    "\xce\xb1"}, {"beta","\xce\xb2"},
    {"gamma",    "\xce\xb3"}, {"delta","\xce\xb4"},
    {"epsilon",  "\xce\xb5"}, {"pi","\xcf\x80"},
    {"sigma",    "\xcf\x83"}, {"omega","\xcf\x89"},
    {"Omega",    "\xce\xa9"}, {"Delta","\xce\x94"},
    /* Common HTML entities */
    {"uml",      "\xc2\xa8"}, {"circ","\xcb\x86"},
    {"tilde",    "\xcb\x9c"},
    {"fnof",     "\xc6\x92"}, {"infin","\xe2\x88\x9e"},
    {"ne",       "\xe2\x89\xa0"}, {"le","\xe2\x89\xa4"},
    {"ge",       "\xe2\x89\xa5"}, {"sum","\xe2\x88\x91"},
    {"radic",    "\xe2\x88\x9a"}, {"empty","\xe2\x88\x85"},
    {"exist",    "\xe2\x88\x83"}, {"forall","\xe2\x88\x80"},
    {"part",     "\xe2\x88\x82"}, {"nabla","\xe2\x88\x87"},
    {"isin",     "\xe2\x88\x88"}, {"notin","\xe2\x88\x89"},
    {"and",      "\xe2\x88\xa7"}, {"or","\xe2\x88\xa8"},
    {"oplus",    "\xe2\x8a\x95"}, {"otimes","\xe2\x8a\x97"},
    {"perp",     "\xe2\x8a\xa5"},
    {NULL, NULL}
};

/* Encode a Unicode codepoint as UTF-8 into buf. Returns bytes written. */
static int encode_utf8(unsigned int cp, char *buf)
{
    if (cp < 0x80) {
        buf[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        buf[0] = (char)(0xC0 | (cp >> 6));
        buf[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        buf[0] = (char)(0xE0 | (cp >> 12));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp < 0x110000) {
        buf[0] = (char)(0xF0 | (cp >> 18));
        buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    buf[0] = '?';
    return 1;
}

/* Forward declaration — defined after at_eof/peek/consume. */
static int try_decode_charref(HtmlTokenizer *t, char *buf, size_t buf_cap);

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

/* ── Character reference decoder (uses at_eof/peek/consume) ───────── */

static int try_decode_charref(HtmlTokenizer *t, char *buf, size_t buf_cap)
{
    size_t start_pos = t->pos;

    if (at_eof(t)) return 0;

    /* Numeric reference: &#... */
    if (peek(t) == '#') {
        consume(t);
        unsigned int cp = 0;
        int digits = 0;

        if (!at_eof(t) && (peek(t) == 'x' || peek(t) == 'X')) {
            consume(t);
            while (!at_eof(t) && digits < 8) {
                char c = peek(t);
                if (c >= '0' && c <= '9')      { cp = cp * 16 + (c - '0'); }
                else if (c >= 'a' && c <= 'f') { cp = cp * 16 + (c - 'a' + 10); }
                else if (c >= 'A' && c <= 'F') { cp = cp * 16 + (c - 'A' + 10); }
                else break;
                consume(t);
                digits++;
            }
        } else {
            while (!at_eof(t) && digits < 10) {
                char c = peek(t);
                if (c >= '0' && c <= '9') { cp = cp * 10 + (c - '0'); }
                else break;
                consume(t);
                digits++;
            }
        }

        if (!at_eof(t) && peek(t) == ';') consume(t);

        if (digits > 0 && cp > 0 && cp < 0x110000) {
            return encode_utf8(cp, buf);
        }
        t->pos = start_pos;
        return 0;
    }

    /* Named reference: &name; */
    char name[32];
    int name_len = 0;
    while (!at_eof(t) && name_len < 30) {
        char c = peek(t);
        if (c == ';') { consume(t); break; }
        if (!isalnum((unsigned char)c)) break;
        name[name_len++] = c;
        consume(t);
    }
    name[name_len] = '\0';

    if (name_len > 0) {
        for (const NamedEntity *e = entities; e->name; e++) {
            if (strcmp(e->name, name) == 0) {
                int vlen = (int)strlen(e->value);
                if ((size_t)vlen <= buf_cap) {
                    memcpy(buf, e->value, vlen);
                    return vlen;
                }
            }
        }
    }

    t->pos = start_pos;
    return 0;
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
            if (c == '&') {
                char decoded[8];
                int n = try_decode_charref(t, decoded, sizeof(decoded));
                if (n > 0) {
                    out->type = TOK_CHARACTER;
                    memcpy(out->ch_data, decoded, n);
                    out->ch_len = n;
                    return true;
                }
            }
            emit_char_token(out, c);
            return true;
        }

        case STATE_RCDATA: {
            if (at_eof(t)) { emit_eof(out); return false; }
            char c = consume(t);
            if (c == '<') { t->state = STATE_RCDATA_LESS_THAN; continue; }
            if (c == '&') {
                char decoded[8];
                int n = try_decode_charref(t, decoded, sizeof(decoded));
                if (n > 0) {
                    out->type = TOK_CHARACTER;
                    memcpy(out->ch_data, decoded, n);
                    out->ch_len = n;
                    return true;
                }
            }
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
            if (c == '&') {
                char decoded[8];
                int n = try_decode_charref(t, decoded, sizeof(decoded));
                if (n > 0) {
                    for (int i = 0; i < n && t->attr_value_len < sizeof(t->attr_value_buf) - 1; i++)
                        t->attr_value_buf[t->attr_value_len++] = decoded[i];
                    continue;
                }
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
            if (c == '&') {
                char decoded[8];
                int n = try_decode_charref(t, decoded, sizeof(decoded));
                if (n > 0) {
                    for (int i = 0; i < n && t->attr_value_len < sizeof(t->attr_value_buf) - 1; i++)
                        t->attr_value_buf[t->attr_value_len++] = decoded[i];
                    continue;
                }
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
