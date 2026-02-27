/*
 * quickjs.c — Compact JavaScript interpreter for Pane browser.
 *
 * Implements the QuickJS-compatible C API declared in quickjs.h.
 * Uses a recursive-descent parser with direct tree-walk evaluation.
 */

#include "quickjs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

/* ================================================================
 * Section 1: Internal Data Structures
 * ================================================================ */

#define MAX_PROPS 256
#define MAX_STACK 64
#define MAX_SCOPE_DEPTH 64
#define INTERRUPT_CHECK_INTERVAL 1000

/* Internal string */
typedef struct JSString_s {
    char *data;
    size_t len;
    int ref;
} JSString_s;

/* Property entry */
typedef struct JSProp {
    char *name;
    JSValue value;
    struct JSProp *next;
} JSProp;

/* Internal object */
typedef struct JSObject_s {
    JSProp *props;
    int ref;
    /* Array storage */
    JSValue *array_data;
    int array_len;
    int array_cap;
    /* Prototype chain (simplified) */
    struct JSObject_s *proto;
} JSObject_s;

/* Function types */
typedef enum { FUNC_C, FUNC_JS } FuncType;

/* Internal function */
typedef struct JSFunc_s {
    FuncType type;
    int ref;
    char *name;
    union {
        struct {
            JSCFunction callback;
            int length;
        } c;
        struct {
            char *source;     /* function body source */
            size_t source_len;
            char **params;    /* parameter names */
            int param_count;
            struct Scope *closure; /* captured scope */
        } js;
    } u;
} JSFunc_s;

/* Scope (variable environment) */
typedef struct Scope {
    JSProp *vars;
    struct Scope *parent;
    int ref;
} Scope;

/* ── Runtime ── */
struct JSRuntime {
    size_t memory_limit;
    size_t memory_used;
    size_t max_stack_size;
    JSInterruptHandler interrupt_handler;
    void *interrupt_opaque;
};

/* ── Context ── */
struct JSContext {
    JSRuntime *rt;
    JSValue global_obj;
    JSValue current_exception;
    int has_exception;
    void *opaque;
    JSExecStats stats;
    /* Scope chain */
    Scope *current_scope;
    int scope_depth;
    /* Output buffer for browser globals */
    char *output_buf;
    size_t output_len;
    size_t output_cap;
    /* Statement counter for interrupt checks */
    int stmt_counter;
    /* Return value mechanism (per-context, not global) */
    JSValue return_value;
    int has_return;
};

/* ================================================================
 * Section 2: Memory Management
 * ================================================================ */

static void *js_malloc(JSRuntime *rt, size_t size) {
    if (rt && rt->memory_limit > 0 &&
        rt->memory_used + size > rt->memory_limit) {
        return NULL;
    }
    void *p = malloc(size);
    if (p && rt) rt->memory_used += size;
    return p;
}

static void *js_realloc(JSRuntime *rt, void *ptr, size_t old_size, size_t new_size) {
    if (rt && rt->memory_limit > 0 &&
        rt->memory_used - old_size + new_size > rt->memory_limit) {
        return NULL;
    }
    void *p = realloc(ptr, new_size);
    if (p && rt) rt->memory_used += new_size - old_size;
    return p;
}

static void js_free(JSRuntime *rt, void *ptr, size_t size) {
    if (ptr) {
        free(ptr);
        if (rt) rt->memory_used -= size;
    }
}

static char *js_strdup(JSRuntime *rt, const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *d = (char *)js_malloc(rt, len);
    if (d) memcpy(d, s, len);
    return d;
}

static char *js_strndup(JSRuntime *rt, const char *s, size_t n) {
    char *d = (char *)js_malloc(rt, n + 1);
    if (d) {
        memcpy(d, s, n);
        d[n] = '\0';
    }
    return d;
}

/* ================================================================
 * Section 3: Internal Value Helpers
 * ================================================================ */

/* Create a JSValue holding a heap-allocated string */
static JSValue js_mkstring(JSContext *ctx, const char *str, size_t len) {
    JSString_s *s = (JSString_s *)js_malloc(ctx->rt, sizeof(JSString_s));
    if (!s) return JS_EXCEPTION;
    s->data = js_strndup(ctx->rt, str, len);
    if (!s->data) { js_free(ctx->rt, s, sizeof(JSString_s)); return JS_EXCEPTION; }
    s->len = len;
    s->ref = 1;
    return JS_MKVAL(JS_TAG_STRING, (uintptr_t)s);
}

/* Create a JSValue holding a heap-allocated object */
static JSValue js_mkobject(JSContext *ctx) {
    JSObject_s *o = (JSObject_s *)js_malloc(ctx->rt, sizeof(JSObject_s));
    if (!o) return JS_EXCEPTION;
    memset(o, 0, sizeof(JSObject_s));
    o->ref = 1;
    return JS_MKVAL(JS_TAG_OBJECT, (uintptr_t)o);
}

/* Create a JSValue holding a heap-allocated array */
static JSValue js_mkarray(JSContext *ctx) {
    JSObject_s *o = (JSObject_s *)js_malloc(ctx->rt, sizeof(JSObject_s));
    if (!o) return JS_EXCEPTION;
    memset(o, 0, sizeof(JSObject_s));
    o->ref = 1;
    o->array_cap = 8;
    o->array_data = (JSValue *)js_malloc(ctx->rt, sizeof(JSValue) * 8);
    if (!o->array_data) { js_free(ctx->rt, o, sizeof(JSObject_s)); return JS_EXCEPTION; }
    o->array_len = 0;
    return JS_MKVAL(JS_TAG_ARRAY, (uintptr_t)o);
}

/* Create a JSValue holding a function */
static JSValue js_mkfunc_c(JSContext *ctx, JSCFunction *func,
                           const char *name, int length) {
    JSFunc_s *f = (JSFunc_s *)js_malloc(ctx->rt, sizeof(JSFunc_s));
    if (!f) return JS_EXCEPTION;
    memset(f, 0, sizeof(JSFunc_s));
    f->type = FUNC_C;
    f->ref = 1;
    f->name = js_strdup(ctx->rt, name ? name : "");
    if (!f->name) { js_free(ctx->rt, f, sizeof(JSFunc_s)); return JS_EXCEPTION; }
    f->u.c.callback = func ? *func : NULL;
    f->u.c.length = length;
    return JS_MKVAL(JS_TAG_C_FUNCTION, (uintptr_t)f);
}

static JSValue js_mkfunc_js(JSContext *ctx, const char *source, size_t source_len,
                            char **params, int param_count, Scope *closure) {
    JSFunc_s *f = (JSFunc_s *)js_malloc(ctx->rt, sizeof(JSFunc_s));
    if (!f) return JS_EXCEPTION;
    memset(f, 0, sizeof(JSFunc_s));
    f->type = FUNC_JS;
    f->ref = 1;
    f->name = js_strdup(ctx->rt, "");
    if (!f->name) { js_free(ctx->rt, f, sizeof(JSFunc_s)); return JS_EXCEPTION; }
    f->u.js.source = js_strndup(ctx->rt, source, source_len);
    if (!f->u.js.source) {
        js_free(ctx->rt, f->name, 1);
        js_free(ctx->rt, f, sizeof(JSFunc_s));
        return JS_EXCEPTION;
    }
    f->u.js.source_len = source_len;
    f->u.js.params = params;
    f->u.js.param_count = param_count;
    f->u.js.closure = closure;
    if (closure) closure->ref++;
    return JS_MKVAL(JS_TAG_FUNCTION, (uintptr_t)f);
}

/* Get internal string pointer */
static JSString_s *js_get_string(JSValue v) {
    if (JS_VALUE_GET_TAG(v) != JS_TAG_STRING) return NULL;
    return (JSString_s *)JS_VALUE_GET_PTR(v);
}

/* Get internal object pointer */
static JSObject_s *js_get_object(JSValue v) {
    int tag = JS_VALUE_GET_TAG(v);
    if (tag != JS_TAG_OBJECT && tag != JS_TAG_ARRAY) return NULL;
    return (JSObject_s *)JS_VALUE_GET_PTR(v);
}

/* Get internal function pointer */
static JSFunc_s *js_get_func(JSValue v) {
    int tag = JS_VALUE_GET_TAG(v);
    if (tag != JS_TAG_FUNCTION && tag != JS_TAG_C_FUNCTION) return NULL;
    return (JSFunc_s *)JS_VALUE_GET_PTR(v);
}

/* Forward declarations */
static JSValue js_eval_source(JSContext *ctx, const char *input, size_t len);
static void js_free_prop_list(JSRuntime *rt, JSProp *p);
static void scope_free(JSRuntime *rt, Scope *scope);
static int scope_define(JSRuntime *rt, Scope *scope, const char *name, JSValue val);

/* Return value mechanism is now per-context: ctx->return_value / ctx->has_return */

/* ================================================================
 * Section 4: Float64 Storage
 * ================================================================
 * Since NaN-boxing with 48-bit payload can't hold a full 64-bit double,
 * we store float64 values as heap-allocated doubles.
 */

typedef struct JSFloat_s {
    double value;
    int ref;
} JSFloat_s;

static JSValue js_mkfloat(JSContext *ctx, double val) {
    /* If value fits in int32, store as int */
    if (val == (double)(int32_t)val && !isnan(val) &&
        val != 0.0 /* avoid -0 */) {
        return JS_MKVAL(JS_TAG_INT, (uint64_t)(uint32_t)(int32_t)val);
    }
    JSFloat_s *f = (JSFloat_s *)js_malloc(ctx->rt, sizeof(JSFloat_s));
    if (!f) return JS_EXCEPTION;
    f->value = val;
    f->ref = 1;
    return JS_MKVAL(JS_TAG_FLOAT64, (uintptr_t)f);
}

static double js_get_float64(JSValue v) {
    if (JS_VALUE_GET_TAG(v) == JS_TAG_INT) {
        return (double)JS_VALUE_GET_INT(v);
    }
    if (JS_VALUE_GET_TAG(v) == JS_TAG_FLOAT64) {
        JSFloat_s *f = (JSFloat_s *)JS_VALUE_GET_PTR(v);
        return f ? f->value : NAN;
    }
    return NAN;
}

static double js_value_to_number(JSValue v) {
    int tag = JS_VALUE_GET_TAG(v);
    if (tag == JS_TAG_INT) return (double)JS_VALUE_GET_INT(v);
    if (tag == JS_TAG_FLOAT64) return js_get_float64(v);
    if (tag == JS_TAG_BOOL) return JS_VALUE_GET_BOOL(v) ? 1.0 : 0.0;
    if (tag == JS_TAG_NULL) return 0.0;
    if (tag == JS_TAG_UNDEFINED) return NAN;
    if (tag == JS_TAG_STRING) {
        JSString_s *s = js_get_string(v);
        if (s && s->data && s->len > 0) {
            char *end;
            double d = strtod(s->data, &end);
            if (*end == '\0') return d;
        }
        return NAN;
    }
    return NAN;
}


/* ================================================================
 * Section 5: Scope Management
 * ================================================================ */

static Scope *scope_new(JSRuntime *rt, Scope *parent) {
    Scope *s = (Scope *)js_malloc(rt, sizeof(Scope));
    if (!s) return NULL;
    s->vars = NULL;
    s->parent = parent;
    s->ref = 1;
    if (parent) parent->ref++;
    return s;
}

static void scope_free(JSRuntime *rt, Scope *scope) {
    if (!scope) return;
    scope->ref--;
    if (scope->ref > 0) return;
    js_free_prop_list(rt, scope->vars);
    if (scope->parent) scope_free(rt, scope->parent);
    js_free(rt, scope, sizeof(Scope));
}

/* Forward decl for free_value used in prop list freeing */
static void js_free_value_rt(JSRuntime *rt, JSValue val);

static void js_free_prop_list(JSRuntime *rt, JSProp *p) {
    while (p) {
        JSProp *next = p->next;
        js_free_value_rt(rt, p->value);
        js_free(rt, p->name, strlen(p->name) + 1);
        js_free(rt, p, sizeof(JSProp));
        p = next;
    }
}

static void js_free_value_rt(JSRuntime *rt, JSValue val) {
    int tag = JS_VALUE_GET_TAG(val);
    switch (tag) {
        case JS_TAG_STRING: {
            JSString_s *s = (JSString_s *)JS_VALUE_GET_PTR(val);
            if (s && --s->ref <= 0) {
                js_free(rt, s->data, s->len + 1);
                js_free(rt, s, sizeof(JSString_s));
            }
            break;
        }
        case JS_TAG_FLOAT64: {
            JSFloat_s *f = (JSFloat_s *)JS_VALUE_GET_PTR(val);
            if (f && --f->ref <= 0) {
                js_free(rt, f, sizeof(JSFloat_s));
            }
            break;
        }
        case JS_TAG_OBJECT:
        case JS_TAG_ARRAY: {
            JSObject_s *o = (JSObject_s *)JS_VALUE_GET_PTR(val);
            if (o && --o->ref <= 0) {
                js_free_prop_list(rt, o->props);
                if (o->array_data) {
                    for (int i = 0; i < o->array_len; i++)
                        js_free_value_rt(rt, o->array_data[i]);
                    js_free(rt, o->array_data, sizeof(JSValue) * o->array_cap);
                }
                /* Free prototype chain (decrement refcount) */
                if (o->proto) {
                    o->proto->ref--;
                    if (o->proto->ref <= 0) {
                        /* Create a temporary value to free the proto object */
                        JSValue proto_val = JS_MKVAL(JS_TAG_OBJECT, (uintptr_t)o->proto);
                        js_free_value_rt(rt, proto_val);
                    }
                }
                js_free(rt, o, sizeof(JSObject_s));
            }
            break;
        }
        case JS_TAG_FUNCTION:
        case JS_TAG_C_FUNCTION: {
            JSFunc_s *f = (JSFunc_s *)JS_VALUE_GET_PTR(val);
            if (f && --f->ref <= 0) {
                js_free(rt, f->name, strlen(f->name) + 1);
                if (f->type == FUNC_JS) {
                    js_free(rt, f->u.js.source, f->u.js.source_len + 1);
                    for (int i = 0; i < f->u.js.param_count; i++)
                        js_free(rt, f->u.js.params[i],
                                strlen(f->u.js.params[i]) + 1);
                    if (f->u.js.params)
                        js_free(rt, f->u.js.params,
                                sizeof(char *) * f->u.js.param_count);
                    if (f->u.js.closure)
                        scope_free(rt, f->u.js.closure);
                }
                js_free(rt, f, sizeof(JSFunc_s));
            }
            break;
        }
        default:
            break;
    }
}

static JSValue js_dup_value(JSValue val) {
    int tag = JS_VALUE_GET_TAG(val);
    switch (tag) {
        case JS_TAG_STRING: {
            JSString_s *s = (JSString_s *)JS_VALUE_GET_PTR(val);
            if (s) s->ref++;
            break;
        }
        case JS_TAG_FLOAT64: {
            JSFloat_s *f = (JSFloat_s *)JS_VALUE_GET_PTR(val);
            if (f) f->ref++;
            break;
        }
        case JS_TAG_OBJECT:
        case JS_TAG_ARRAY: {
            JSObject_s *o = (JSObject_s *)JS_VALUE_GET_PTR(val);
            if (o) o->ref++;
            break;
        }
        case JS_TAG_FUNCTION:
        case JS_TAG_C_FUNCTION: {
            JSFunc_s *f = (JSFunc_s *)JS_VALUE_GET_PTR(val);
            if (f) f->ref++;
            break;
        }
        default:
            break;
    }
    return val;
}

/* ================================================================
 * Section 6: Object Property Operations
 * ================================================================ */

static JSValue obj_get_prop(JSObject_s *o, const char *name) {
    if (!o) return JS_UNDEFINED;
    /* Check array "length" */
    if (o->array_data && strcmp(name, "length") == 0) {
        return JS_MKVAL(JS_TAG_INT, (uint64_t)(uint32_t)o->array_len);
    }
    for (JSProp *p = o->props; p; p = p->next) {
        if (strcmp(p->name, name) == 0) return js_dup_value(p->value);
    }
    if (o->proto) return obj_get_prop(o->proto, name);
    return JS_UNDEFINED;
}

static int obj_set_prop(JSRuntime *rt, JSObject_s *o, const char *name,
                        JSValue val) {
    if (!o) return -1;
    /* Update existing */
    for (JSProp *p = o->props; p; p = p->next) {
        if (strcmp(p->name, name) == 0) {
            js_free_value_rt(rt, p->value);
            p->value = val;
            return 0;
        }
    }
    /* Add new */
    JSProp *p = (JSProp *)js_malloc(rt, sizeof(JSProp));
    if (!p) { js_free_value_rt(rt, val); return -1; }
    p->name = js_strdup(rt, name);
    if (!p->name) { js_free(rt, p, sizeof(JSProp)); js_free_value_rt(rt, val); return -1; }
    p->value = val;
    p->next = o->props;
    o->props = p;
    return 0;
}

static int obj_delete_prop(JSRuntime *rt, JSObject_s *o, const char *name) {
    if (!o) return -1;
    JSProp **pp = &o->props;
    while (*pp) {
        if (strcmp((*pp)->name, name) == 0) {
            JSProp *p = *pp;
            *pp = p->next;
            js_free_value_rt(rt, p->value);
            js_free(rt, p->name, strlen(p->name) + 1);
            js_free(rt, p, sizeof(JSProp));
            return 1;
        }
        pp = &(*pp)->next;
    }
    return 0;
}

static int obj_has_prop(JSObject_s *o, const char *name) {
    if (!o) return 0;
    for (JSProp *p = o->props; p; p = p->next) {
        if (strcmp(p->name, name) == 0) return 1;
    }
    return 0;
}

/* Scope variable lookup */
static JSValue scope_get(Scope *scope, const char *name) {
    for (Scope *s = scope; s; s = s->parent) {
        for (JSProp *p = s->vars; p; p = p->next) {
            if (strcmp(p->name, name) == 0) return js_dup_value(p->value);
        }
    }
    return JS_UNDEFINED;
}

/* Scope variable set (finds existing or creates in current scope) */
static int scope_set(JSRuntime *rt, Scope *scope, const char *name,
                     JSValue val) {
    /* Search up the chain for existing var */
    for (Scope *s = scope; s; s = s->parent) {
        for (JSProp *p = s->vars; p; p = p->next) {
            if (strcmp(p->name, name) == 0) {
                js_free_value_rt(rt, p->value);
                p->value = val;
                return 0;
            }
        }
    }
    /* Create in current scope */
    return scope_define(rt, scope, name, val);
}

/* Define variable in current scope only */
static int scope_define(JSRuntime *rt, Scope *scope, const char *name,
                        JSValue val) {
    /* Check if already exists in this scope */
    for (JSProp *p = scope->vars; p; p = p->next) {
        if (strcmp(p->name, name) == 0) {
            js_free_value_rt(rt, p->value);
            p->value = val;
            return 0;
        }
    }
    JSProp *p = (JSProp *)js_malloc(rt, sizeof(JSProp));
    if (!p) { js_free_value_rt(rt, val); return -1; }
    p->name = js_strdup(rt, name);
    if (!p->name) { js_free(rt, p, sizeof(JSProp)); js_free_value_rt(rt, val); return -1; }
    p->value = val;
    p->next = scope->vars;
    scope->vars = p;
    return 0;
}


/* ================================================================
 * Section 7: Lexer (Tokenizer)
 * ================================================================ */

typedef enum {
    TOK_EOF = 0,
    TOK_NUMBER, TOK_STRING, TOK_TEMPLATE, TOK_IDENT, TOK_REGEXP,
    /* Keywords */
    TOK_VAR, TOK_LET, TOK_CONST, TOK_FUNCTION, TOK_RETURN,
    TOK_IF, TOK_ELSE, TOK_FOR, TOK_WHILE, TOK_DO,
    TOK_BREAK, TOK_CONTINUE, TOK_SWITCH, TOK_CASE, TOK_DEFAULT,
    TOK_TRY, TOK_CATCH, TOK_FINALLY, TOK_THROW,
    TOK_NEW, TOK_DELETE, TOK_TYPEOF, TOK_VOID, TOK_INSTANCEOF, TOK_IN,
    TOK_THIS, TOK_TRUE, TOK_FALSE, TOK_NULL, TOK_UNDEFINED,
    TOK_CLASS, TOK_EXTENDS, TOK_SUPER,
    /* Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT, TOK_POWER,
    TOK_EQ, TOK_NEQ, TOK_SEQ, TOK_SNEQ,
    TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT, TOK_NULLISH,
    TOK_BAND, TOK_BOR, TOK_BXOR, TOK_BNOT, TOK_SHL, TOK_SHR, TOK_USHR,
    TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN, TOK_PERCENT_ASSIGN,
    TOK_INC, TOK_DEC,
    TOK_QUESTION, TOK_COLON, TOK_COMMA, TOK_SEMICOLON, TOK_DOT,
    TOK_ARROW, TOK_SPREAD,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_OPTIONAL_CHAIN,  /* ?. */
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    double num_val;
    int line;
} Token;

typedef struct {
    const char *src;
    int pos;
    int len;
    int line;
    Token current;
    Token peek;
    int has_peek;
} Lexer;

static void lexer_init(Lexer *lex, const char *src, int len) {
    lex->src = src;
    lex->pos = 0;
    lex->len = len;
    lex->line = 1;
    lex->has_peek = 0;
}

static int lex_eof(Lexer *lex) { return lex->pos >= lex->len; }
static char lex_ch(Lexer *lex) {
    return lex->pos < lex->len ? lex->src[lex->pos] : '\0';
}
static char lex_peek_ch(Lexer *lex, int offset) {
    int p = lex->pos + offset;
    return p < lex->len ? lex->src[p] : '\0';
}
static void lex_advance(Lexer *lex) {
    if (lex->pos < lex->len) {
        if (lex->src[lex->pos] == '\n') lex->line++;
        lex->pos++;
    }
}

static void skip_whitespace_comments(Lexer *lex) {
    while (!lex_eof(lex)) {
        char c = lex_ch(lex);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lex_advance(lex);
        } else if (c == '/' && lex_peek_ch(lex, 1) == '/') {
            /* Single-line comment */
            while (!lex_eof(lex) && lex_ch(lex) != '\n') lex_advance(lex);
        } else if (c == '/' && lex_peek_ch(lex, 1) == '*') {
            /* Multi-line comment */
            lex_advance(lex); lex_advance(lex);
            while (!lex_eof(lex)) {
                if (lex_ch(lex) == '*' && lex_peek_ch(lex, 1) == '/') {
                    lex_advance(lex); lex_advance(lex);
                    break;
                }
                lex_advance(lex);
            }
        } else {
            break;
        }
    }
}

static Token lex_string(Lexer *lex, char quote) {
    Token tok;
    tok.type = (quote == '`') ? TOK_TEMPLATE : TOK_STRING;
    tok.line = lex->line;
    lex_advance(lex); /* skip opening quote */
    tok.start = lex->src + lex->pos;
    int start = lex->pos;

    while (!lex_eof(lex) && lex_ch(lex) != quote) {
        if (lex_ch(lex) == '\\') lex_advance(lex); /* skip escape */
        lex_advance(lex);
    }
    tok.len = lex->pos - start;
    if (!lex_eof(lex)) lex_advance(lex); /* skip closing quote */
    return tok;
}

static Token lex_number(Lexer *lex) {
    Token tok;
    tok.type = TOK_NUMBER;
    tok.line = lex->line;
    tok.start = lex->src + lex->pos;
    int start = lex->pos;

    /* Handle 0x, 0o, 0b prefixes */
    if (lex_ch(lex) == '0' && lex->pos + 1 < lex->len) {
        char next = lex_peek_ch(lex, 1);
        if (next == 'x' || next == 'X') {
            lex_advance(lex); lex_advance(lex);
            while (!lex_eof(lex) && isxdigit(lex_ch(lex))) lex_advance(lex);
            tok.len = lex->pos - start;
            tok.num_val = (double)strtol(tok.start, NULL, 16);
            return tok;
        }
    }

    while (!lex_eof(lex) && isdigit(lex_ch(lex))) lex_advance(lex);
    if (!lex_eof(lex) && lex_ch(lex) == '.' && isdigit(lex_peek_ch(lex, 1))) {
        lex_advance(lex); /* skip . */
        while (!lex_eof(lex) && isdigit(lex_ch(lex))) lex_advance(lex);
    }
    /* Exponent */
    if (!lex_eof(lex) && (lex_ch(lex) == 'e' || lex_ch(lex) == 'E')) {
        lex_advance(lex);
        if (!lex_eof(lex) && (lex_ch(lex) == '+' || lex_ch(lex) == '-'))
            lex_advance(lex);
        while (!lex_eof(lex) && isdigit(lex_ch(lex))) lex_advance(lex);
    }
    tok.len = lex->pos - start;
    tok.num_val = strtod(tok.start, NULL);
    return tok;
}

static int is_ident_start(char c) {
    return isalpha(c) || c == '_' || c == '$';
}
static int is_ident_char(char c) {
    return isalnum(c) || c == '_' || c == '$';
}

static TokenType keyword_lookup(const char *s, int len) {
    #define KW(str, tok) if (len == (int)sizeof(str)-1 && memcmp(s, str, len)==0) return tok
    KW("var", TOK_VAR); KW("let", TOK_LET); KW("const", TOK_CONST);
    KW("function", TOK_FUNCTION); KW("return", TOK_RETURN);
    KW("if", TOK_IF); KW("else", TOK_ELSE);
    KW("for", TOK_FOR); KW("while", TOK_WHILE); KW("do", TOK_DO);
    KW("break", TOK_BREAK); KW("continue", TOK_CONTINUE);
    KW("switch", TOK_SWITCH); KW("case", TOK_CASE); KW("default", TOK_DEFAULT);
    KW("try", TOK_TRY); KW("catch", TOK_CATCH); KW("finally", TOK_FINALLY);
    KW("throw", TOK_THROW);
    KW("new", TOK_NEW); KW("delete", TOK_DELETE);
    KW("typeof", TOK_TYPEOF); KW("void", TOK_VOID);
    KW("instanceof", TOK_INSTANCEOF); KW("in", TOK_IN);
    KW("this", TOK_THIS);
    KW("true", TOK_TRUE); KW("false", TOK_FALSE);
    KW("null", TOK_NULL); KW("undefined", TOK_UNDEFINED);
    KW("class", TOK_CLASS); KW("extends", TOK_EXTENDS); KW("super", TOK_SUPER);
    #undef KW
    return TOK_IDENT;
}

static Token lex_next(Lexer *lex) {
    if (lex->has_peek) {
        lex->has_peek = 0;
        lex->current = lex->peek;
        return lex->current;
    }

    skip_whitespace_comments(lex);

    Token tok;
    tok.num_val = 0;
    tok.line = lex->line;

    if (lex_eof(lex)) {
        tok.type = TOK_EOF; tok.start = ""; tok.len = 0;
        lex->current = tok;
        return tok;
    }

    char c = lex_ch(lex);

    /* Strings */
    if (c == '"' || c == '\'' || c == '`') {
        tok = lex_string(lex, c);
        lex->current = tok;
        return tok;
    }

    /* Numbers */
    if (isdigit(c) || (c == '.' && isdigit(lex_peek_ch(lex, 1)))) {
        tok = lex_number(lex);
        lex->current = tok;
        return tok;
    }

    /* Identifiers and keywords */
    if (is_ident_start(c)) {
        tok.start = lex->src + lex->pos;
        int start = lex->pos;
        while (!lex_eof(lex) && is_ident_char(lex_ch(lex))) lex_advance(lex);
        tok.len = lex->pos - start;
        tok.type = keyword_lookup(tok.start, tok.len);
        lex->current = tok;
        return tok;
    }

    /* Operators and punctuation */
    tok.start = lex->src + lex->pos;
    tok.len = 1;

    switch (c) {
        case '+':
            lex_advance(lex);
            if (lex_ch(lex) == '+') { lex_advance(lex); tok.type = TOK_INC; tok.len = 2; }
            else if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_PLUS_ASSIGN; tok.len = 2; }
            else tok.type = TOK_PLUS;
            break;
        case '-':
            lex_advance(lex);
            if (lex_ch(lex) == '-') { lex_advance(lex); tok.type = TOK_DEC; tok.len = 2; }
            else if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_MINUS_ASSIGN; tok.len = 2; }
            else tok.type = TOK_MINUS;
            break;
        case '*':
            lex_advance(lex);
            if (lex_ch(lex) == '*') { lex_advance(lex); tok.type = TOK_POWER; tok.len = 2; }
            else if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_STAR_ASSIGN; tok.len = 2; }
            else tok.type = TOK_STAR;
            break;
        case '/':
            lex_advance(lex);
            if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_SLASH_ASSIGN; tok.len = 2; }
            else tok.type = TOK_SLASH;
            break;
        case '%':
            lex_advance(lex);
            if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_PERCENT_ASSIGN; tok.len = 2; }
            else tok.type = TOK_PERCENT;
            break;
        case '=':
            lex_advance(lex);
            if (lex_ch(lex) == '=') {
                lex_advance(lex);
                if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_SEQ; tok.len = 3; }
                else { tok.type = TOK_EQ; tok.len = 2; }
            } else if (lex_ch(lex) == '>') {
                lex_advance(lex); tok.type = TOK_ARROW; tok.len = 2;
            } else tok.type = TOK_ASSIGN;
            break;
        case '!':
            lex_advance(lex);
            if (lex_ch(lex) == '=') {
                lex_advance(lex);
                if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_SNEQ; tok.len = 3; }
                else { tok.type = TOK_NEQ; tok.len = 2; }
            } else tok.type = TOK_NOT;
            break;
        case '<':
            lex_advance(lex);
            if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_LE; tok.len = 2; }
            else if (lex_ch(lex) == '<') { lex_advance(lex); tok.type = TOK_SHL; tok.len = 2; }
            else tok.type = TOK_LT;
            break;
        case '>':
            lex_advance(lex);
            if (lex_ch(lex) == '=') { lex_advance(lex); tok.type = TOK_GE; tok.len = 2; }
            else if (lex_ch(lex) == '>') {
                lex_advance(lex);
                if (lex_ch(lex) == '>') { lex_advance(lex); tok.type = TOK_USHR; tok.len = 3; }
                else { tok.type = TOK_SHR; tok.len = 2; }
            }
            else tok.type = TOK_GT;
            break;
        case '&':
            lex_advance(lex);
            if (lex_ch(lex) == '&') { lex_advance(lex); tok.type = TOK_AND; tok.len = 2; }
            else tok.type = TOK_BAND;
            break;
        case '|':
            lex_advance(lex);
            if (lex_ch(lex) == '|') { lex_advance(lex); tok.type = TOK_OR; tok.len = 2; }
            else tok.type = TOK_BOR;
            break;
        case '^': lex_advance(lex); tok.type = TOK_BXOR; break;
        case '~': lex_advance(lex); tok.type = TOK_BNOT; break;
        case '?':
            lex_advance(lex);
            if (lex_ch(lex) == '.') { lex_advance(lex); tok.type = TOK_OPTIONAL_CHAIN; tok.len = 2; }
            else if (lex_ch(lex) == '?') { lex_advance(lex); tok.type = TOK_NULLISH; tok.len = 2; }
            else tok.type = TOK_QUESTION;
            break;
        case ':': lex_advance(lex); tok.type = TOK_COLON; break;
        case ',': lex_advance(lex); tok.type = TOK_COMMA; break;
        case ';': lex_advance(lex); tok.type = TOK_SEMICOLON; break;
        case '.':
            lex_advance(lex);
            if (lex_ch(lex) == '.' && lex_peek_ch(lex, 1) == '.') {
                lex_advance(lex); lex_advance(lex);
                tok.type = TOK_SPREAD; tok.len = 3;
            } else tok.type = TOK_DOT;
            break;
        case '(': lex_advance(lex); tok.type = TOK_LPAREN; break;
        case ')': lex_advance(lex); tok.type = TOK_RPAREN; break;
        case '{': lex_advance(lex); tok.type = TOK_LBRACE; break;
        case '}': lex_advance(lex); tok.type = TOK_RBRACE; break;
        case '[': lex_advance(lex); tok.type = TOK_LBRACKET; break;
        case ']': lex_advance(lex); tok.type = TOK_RBRACKET; break;
        default:
            lex_advance(lex);
            tok.type = TOK_ERROR;
            break;
    }

    lex->current = tok;
    return tok;
}


/* ================================================================
 * Section 8: Parser + Evaluator (Combined)
 * ================================================================
 * Recursive descent parser that directly evaluates expressions.
 * Function bodies are stored as source strings and re-parsed on call.
 */

/* Forward declarations */
static JSValue eval_expression(JSContext *ctx, Lexer *lex, int min_prec);
static JSValue eval_assignment(JSContext *ctx, Lexer *lex);
static JSValue eval_statement(JSContext *ctx, Lexer *lex);
static JSValue eval_block(JSContext *ctx, Lexer *lex);
static JSValue eval_statement_list(JSContext *ctx, Lexer *lex);

/* ── Token-level skip functions (advance lexer without evaluating) ── */

/* Skip balanced parentheses: expects lexer on '(', leaves after ')' */
static void skip_parens(Lexer *lex) {
    if (lex->current.type != TOK_LPAREN) return;
    int depth = 1;
    lex_next(lex);
    while (!lex_eof(lex) && depth > 0) {
        if (lex->current.type == TOK_LPAREN) depth++;
        else if (lex->current.type == TOK_RPAREN) depth--;
        if (depth > 0) lex_next(lex);
    }
    if (lex->current.type == TOK_RPAREN) lex_next(lex);
}

/* Skip a balanced block: expects lexer on '{', leaves after '}' */
static void skip_block(Lexer *lex) {
    if (lex->current.type != TOK_LBRACE) return;
    int depth = 1;
    lex_next(lex);
    while (!lex_eof(lex) && depth > 0) {
        if (lex->current.type == TOK_LBRACE) depth++;
        else if (lex->current.type == TOK_RBRACE) depth--;
        if (depth > 0) lex_next(lex);
    }
    if (lex->current.type == TOK_RBRACE) lex_next(lex);
}

/* Skip a single statement without evaluating it.
 * Handles blocks, if/else, for, while, do-while, try/catch/finally,
 * switch, and simple expression statements. */
static void skip_statement(Lexer *lex) {
    if (lex_eof(lex)) return;

    /* Block */
    if (lex->current.type == TOK_LBRACE) {
        skip_block(lex);
        return;
    }

    /* if/else */
    if (lex->current.type == TOK_IF) {
        lex_next(lex);
        skip_parens(lex);
        skip_statement(lex);
        if (lex->current.type == TOK_ELSE) {
            lex_next(lex);
            skip_statement(lex);
        }
        return;
    }

    /* for / while */
    if (lex->current.type == TOK_FOR || lex->current.type == TOK_WHILE) {
        lex_next(lex);
        skip_parens(lex);
        skip_statement(lex);
        return;
    }

    /* do-while */
    if (lex->current.type == TOK_DO) {
        lex_next(lex);
        skip_statement(lex);
        if (lex->current.type == TOK_WHILE) {
            lex_next(lex);
            skip_parens(lex);
        }
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return;
    }

    /* try/catch/finally */
    if (lex->current.type == TOK_TRY) {
        lex_next(lex);
        skip_block(lex);
        if (lex->current.type == TOK_CATCH) {
            lex_next(lex);
            if (lex->current.type == TOK_LPAREN) skip_parens(lex);
            skip_block(lex);
        }
        if (lex->current.type == TOK_FINALLY) {
            lex_next(lex);
            skip_block(lex);
        }
        return;
    }

    /* switch */
    if (lex->current.type == TOK_SWITCH) {
        lex_next(lex);
        skip_parens(lex);
        skip_block(lex);
        return;
    }

    /* Simple statement: consume until ';' or '}' (don't consume '}') */
    {
        int depth = 0;
        while (!lex_eof(lex)) {
            if (lex->current.type == TOK_EOF) break;
            if (lex->current.type == TOK_LBRACE) depth++;
            else if (lex->current.type == TOK_RBRACE) {
                if (depth > 0) depth--;
                else break;
            } else if (lex->current.type == TOK_SEMICOLON && depth == 0) {
                lex_next(lex);
                return;
            }
            lex_next(lex);
        }
        /* Normalize EOF state */
        if (lex_eof(lex) && lex->current.type != TOK_EOF) {
            lex_next(lex);
        }
    }
}

/* Skip a single expression without evaluating it.
 * Used for ternary branches that should not execute. */
static void skip_expression(Lexer *lex) {
    int depth = 0;
    while (!lex_eof(lex)) {
        TokenType t = lex->current.type;
        if (t == TOK_EOF) break;
        if (t == TOK_LPAREN || t == TOK_LBRACKET || t == TOK_LBRACE) depth++;
        else if (t == TOK_RPAREN || t == TOK_RBRACKET || t == TOK_RBRACE) {
            if (depth > 0) depth--;
            else break; /* unmatched close — belongs to parent context */
        } else if (depth == 0) {
            /* Stop at expression terminators */
            if (t == TOK_COMMA || t == TOK_SEMICOLON || t == TOK_COLON)
                break;
        }
        lex_next(lex);
    }
    /* Normalize: if lexer position is past the end but current token isn't EOF,
     * advance once more to set current to TOK_EOF */
    if (lex_eof(lex) && lex->current.type != TOK_EOF) {
        lex_next(lex);
    }
}

/* Check if context has an unhandled exception */
static int has_exception(JSContext *ctx) {
    return ctx->has_exception;
}

/* Set an exception on the context */
static JSValue throw_error(JSContext *ctx, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (!ctx->has_exception) {
        ctx->current_exception = js_mkstring(ctx, buf, strlen(buf));
        ctx->has_exception = 1;
        ctx->stats.exceptions_thrown++;
    }
    return JS_EXCEPTION;
}

/* Check interrupt handler */
static int check_interrupt(JSContext *ctx) {
    ctx->stmt_counter++;
    if (ctx->stmt_counter % INTERRUPT_CHECK_INTERVAL == 0) {
        JSRuntime *rt = ctx->rt;
        if (rt->interrupt_handler) {
            if (rt->interrupt_handler(rt, rt->interrupt_opaque)) {
                throw_error(ctx, "execution interrupted");
                return 1;
            }
        }
    }
    return 0;
}

/* Unescape a string literal */
static JSValue parse_string_value(JSContext *ctx, Token *tok) {
    /* Allocate buffer for unescaped string */
    char *buf = (char *)js_malloc(ctx->rt, tok->len + 1);
    if (!buf) return JS_EXCEPTION;
    int j = 0;
    for (int i = 0; i < tok->len; i++) {
        if (tok->start[i] == '\\' && i + 1 < tok->len) {
            i++;
            switch (tok->start[i]) {
                case 'n': buf[j++] = '\n'; break;
                case 't': buf[j++] = '\t'; break;
                case 'r': buf[j++] = '\r'; break;
                case '\\': buf[j++] = '\\'; break;
                case '\'': buf[j++] = '\''; break;
                case '"': buf[j++] = '"'; break;
                case '`': buf[j++] = '`'; break;
                case '0': buf[j++] = '\0'; break;
                case 'x':
                    if (i + 2 < tok->len) {
                        char hex[3] = { tok->start[i+1], tok->start[i+2], 0 };
                        buf[j++] = (char)strtol(hex, NULL, 16);
                        i += 2;
                    }
                    break;
                case 'u':
                    /* Simple unicode escape - just pass through for now */
                    buf[j++] = '\\';
                    buf[j++] = 'u';
                    break;
                default:
                    buf[j++] = tok->start[i];
                    break;
            }
        } else {
            buf[j++] = tok->start[i];
        }
    }
    buf[j] = '\0';
    JSValue result = js_mkstring(ctx, buf, j);
    js_free(ctx->rt, buf, tok->len + 1);
    return result;
}

/* Convert JSValue to string for concatenation */
static JSValue js_tostring(JSContext *ctx, JSValue val) {
    int tag = JS_VALUE_GET_TAG(val);
    switch (tag) {
        case JS_TAG_STRING:
            return js_dup_value(val);
        case JS_TAG_INT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", JS_VALUE_GET_INT(val));
            return js_mkstring(ctx, buf, strlen(buf));
        }
        case JS_TAG_FLOAT64: {
            char buf[64];
            double d = js_get_float64(val);
            if (isnan(d)) return js_mkstring(ctx, "NaN", 3);
            if (isinf(d)) return js_mkstring(ctx, d > 0 ? "Infinity" : "-Infinity",
                                              d > 0 ? 8 : 9);
            snprintf(buf, sizeof(buf), "%.14g", d);
            return js_mkstring(ctx, buf, strlen(buf));
        }
        case JS_TAG_BOOL:
            return js_mkstring(ctx, JS_VALUE_GET_BOOL(val) ? "true" : "false",
                               JS_VALUE_GET_BOOL(val) ? 4 : 5);
        case JS_TAG_NULL:
            return js_mkstring(ctx, "null", 4);
        case JS_TAG_UNDEFINED:
            return js_mkstring(ctx, "undefined", 9);
        case JS_TAG_OBJECT:
            return js_mkstring(ctx, "[object Object]", 15);
        case JS_TAG_ARRAY:
            return js_mkstring(ctx, "", 0); /* simplified */
        case JS_TAG_FUNCTION:
        case JS_TAG_C_FUNCTION:
            return js_mkstring(ctx, "function() { [native code] }", 28);
        default:
            return js_mkstring(ctx, "", 0);
    }
}

/* String concatenation */
static JSValue js_string_concat(JSContext *ctx, JSValue a, JSValue b) {
    JSValue sa = js_tostring(ctx, a);
    if (JS_IsException(sa)) return sa;
    JSValue sb = js_tostring(ctx, b);
    if (JS_IsException(sb)) { JS_FreeValue(ctx, sa); return sb; }

    JSString_s *ssa = js_get_string(sa);
    JSString_s *ssb = js_get_string(sb);
    size_t newlen = ssa->len + ssb->len;
    char *buf = (char *)js_malloc(ctx->rt, newlen + 1);
    if (!buf) {
        JS_FreeValue(ctx, sa); JS_FreeValue(ctx, sb);
        return JS_EXCEPTION;
    }
    memcpy(buf, ssa->data, ssa->len);
    memcpy(buf + ssa->len, ssb->data, ssb->len);
    buf[newlen] = '\0';
    JSValue result = js_mkstring(ctx, buf, newlen);
    js_free(ctx->rt, buf, newlen + 1);
    JS_FreeValue(ctx, sa);
    JS_FreeValue(ctx, sb);
    return result;
}

/* Truthiness check */
static int js_is_truthy(JSValue val) {
    int tag = JS_VALUE_GET_TAG(val);
    switch (tag) {
        case JS_TAG_UNDEFINED:
        case JS_TAG_NULL:
            return 0;
        case JS_TAG_BOOL:
            return JS_VALUE_GET_BOOL(val);
        case JS_TAG_INT:
            return JS_VALUE_GET_INT(val) != 0;
        case JS_TAG_FLOAT64: {
            double d = js_get_float64(val);
            return d != 0.0 && !isnan(d);
        }
        case JS_TAG_STRING: {
            JSString_s *s = js_get_string(val);
            return s && s->len > 0;
        }
        default:
            return 1; /* objects, functions, arrays are truthy */
    }
}

/* Strict equality */
static int js_strict_eq(JSValue a, JSValue b) {
    int ta = JS_VALUE_GET_TAG(a), tb = JS_VALUE_GET_TAG(b);
    if (ta != tb) {
        /* INT and FLOAT64 can be equal */
        if ((ta == JS_TAG_INT || ta == JS_TAG_FLOAT64) &&
            (tb == JS_TAG_INT || tb == JS_TAG_FLOAT64)) {
            return js_value_to_number(a) == js_value_to_number(b);
        }
        return 0;
    }
    switch (ta) {
        case JS_TAG_UNDEFINED:
        case JS_TAG_NULL:
            return 1;
        case JS_TAG_BOOL:
            return JS_VALUE_GET_BOOL(a) == JS_VALUE_GET_BOOL(b);
        case JS_TAG_INT:
            return JS_VALUE_GET_INT(a) == JS_VALUE_GET_INT(b);
        case JS_TAG_FLOAT64:
            return js_get_float64(a) == js_get_float64(b);
        case JS_TAG_STRING: {
            JSString_s *sa = js_get_string(a), *sb = js_get_string(b);
            if (sa->len != sb->len) return 0;
            return memcmp(sa->data, sb->data, sa->len) == 0;
        }
        default:
            /* Object identity */
            return JS_VALUE_GET_PTR(a) == JS_VALUE_GET_PTR(b);
    }
}

/* Abstract equality (==) */
static int js_abstract_eq(JSValue a, JSValue b) {
    int ta = JS_VALUE_GET_TAG(a), tb = JS_VALUE_GET_TAG(b);
    if (ta == tb) return js_strict_eq(a, b);
    /* null == undefined */
    if ((ta == JS_TAG_NULL && tb == JS_TAG_UNDEFINED) ||
        (ta == JS_TAG_UNDEFINED && tb == JS_TAG_NULL))
        return 1;
    /* Number comparisons */
    if ((ta == JS_TAG_INT || ta == JS_TAG_FLOAT64 || ta == JS_TAG_BOOL) &&
        (tb == JS_TAG_INT || tb == JS_TAG_FLOAT64 || tb == JS_TAG_BOOL))
        return js_value_to_number(a) == js_value_to_number(b);
    /* String to number */
    if (ta == JS_TAG_STRING && (tb == JS_TAG_INT || tb == JS_TAG_FLOAT64))
        return js_value_to_number(a) == js_value_to_number(b);
    if (tb == JS_TAG_STRING && (ta == JS_TAG_INT || ta == JS_TAG_FLOAT64))
        return js_value_to_number(a) == js_value_to_number(b);
    return 0;
}


/* ================================================================
 * Section 9: Expression Parser/Evaluator
 * ================================================================ */

/* Special return values for control flow */
#define JS_TAG_RETURN_VALUE  100
#define JS_TAG_BREAK_VALUE   101
#define JS_TAG_CONTINUE_VALUE 102

#define JS_RETURN_VAL(v) JS_MKVAL(JS_TAG_RETURN_VALUE, (uintptr_t)(v))
#define JS_IS_RETURN(v) (JS_VALUE_GET_TAG(v) == JS_TAG_RETURN_VALUE)
#define JS_IS_BREAK(v)  (JS_VALUE_GET_TAG(v) == JS_TAG_BREAK_VALUE)
#define JS_IS_CONTINUE(v) (JS_VALUE_GET_TAG(v) == JS_TAG_CONTINUE_VALUE)
#define JS_BREAK_SIGNAL    JS_MKVAL(JS_TAG_BREAK_VALUE, 0)
#define JS_CONTINUE_SIGNAL JS_MKVAL(JS_TAG_CONTINUE_VALUE, 0)

/* Track object+property for assignment targets */
typedef struct {
    JSValue obj;       /* the parent object (for property assignments) */
    char *prop_name;   /* property name (for property assignments) */
    char *var_name;    /* variable name (for simple variable assignments) */
} LValue;

static void lvalue_clear(JSContext *ctx, LValue *lv) {
    if (lv->prop_name) { js_free(ctx->rt, lv->prop_name, strlen(lv->prop_name)+1); lv->prop_name = NULL; }
    if (lv->var_name) { js_free(ctx->rt, lv->var_name, strlen(lv->var_name)+1); lv->var_name = NULL; }
    if (!JS_IsUndefined(lv->obj)) { JS_FreeValue(ctx, lv->obj); lv->obj = JS_UNDEFINED; }
}

/* Parse primary expression (atoms) */
static JSValue eval_primary(JSContext *ctx, Lexer *lex, LValue *lv) {
    if (has_exception(ctx)) return JS_EXCEPTION;

    Token tok = lex->current;

    switch (tok.type) {
        case TOK_NUMBER: {
            lex_next(lex);
            return js_mkfloat(ctx, tok.num_val);
        }
        case TOK_STRING: {
            lex_next(lex);
            return parse_string_value(ctx, &tok);
        }
        case TOK_TEMPLATE: {
            lex_next(lex);
            /* Simple template literal without interpolation */
            return parse_string_value(ctx, &tok);
        }
        case TOK_TRUE:  lex_next(lex); return JS_TRUE;
        case TOK_FALSE: lex_next(lex); return JS_FALSE;
        case TOK_NULL:  lex_next(lex); return JS_NULL;
        case TOK_UNDEFINED: lex_next(lex); return JS_UNDEFINED;
        case TOK_THIS: {
            lex_next(lex);
            /* 'this' resolves to global in non-strict mode */
            return js_dup_value(ctx->global_obj);
        }
        case TOK_IDENT: {
            char *name = js_strndup(ctx->rt, tok.start, tok.len);
            lex_next(lex);
            /* Check for arrow function: ident => ... */
            if (lex->current.type == TOK_ARROW) {
                lex_next(lex); /* skip => */
                /* Single param arrow function */
                char **params = (char **)js_malloc(ctx->rt, sizeof(char *));
                params[0] = name;
                if (lex->current.type == TOK_LBRACE) {
                    /* Block body: collect source */
                    int start_pos = lex->pos - 1; (void)start_pos;
                    int brace = 1;
                    while (!lex_eof(lex) && brace > 0) {
                        if (lex_ch(lex) == '{') brace++;
                        else if (lex_ch(lex) == '}') brace--;
                        if (brace > 0) lex_advance(lex);
                    }
                    int end_pos = lex->pos;
                    lex_advance(lex); /* skip } */
                    lex_next(lex);
                    return js_mkfunc_js(ctx, lex->src + start_pos,
                                       end_pos - start_pos, params, 1,
                                       ctx->current_scope);
                } else {
                    /* Expression body: evaluate directly */
                    /* skip to end of expression */
                    /* For simplicity, evaluate the expression directly */
                    JSValue body = eval_assignment(ctx, lex);
                    /* Create a function that returns this value */
                    /* Actually, we need to store source for re-evaluation */
                    /* Simplified: evaluate now and wrap */
                    js_free(ctx->rt, params[0], strlen(params[0]) + 1);
                    js_free(ctx->rt, params, sizeof(char *));
                    return body; /* simplified - return the expression value */
                }
            }
            /* Regular identifier - look up variable */
            JSValue val = scope_get(ctx->current_scope, name);
            if (JS_IsUndefined(val)) {
                /* Try global object */
                JSObject_s *gobj = js_get_object(ctx->global_obj);
                if (gobj) val = obj_get_prop(gobj, name);
            }
            if (lv) {
                lv->var_name = name;
            } else {
                js_free(ctx->rt, name, strlen(name) + 1);
            }
            return val;
        }
        case TOK_LPAREN: {
            lex_next(lex); /* skip ( */
            /* Check for arrow function: () => or (params) => */
            /* For now, just parse as grouped expression */
            JSValue val = eval_assignment(ctx, lex);
            if (lex->current.type == TOK_RPAREN) lex_next(lex);
            return val;
        }
        case TOK_LBRACKET: {
            /* Array literal */
            lex_next(lex); /* skip [ */
            JSValue arr = js_mkarray(ctx);
            JSObject_s *o = js_get_object(arr);
            int idx = 0;
            while (lex->current.type != TOK_RBRACKET &&
                   lex->current.type != TOK_EOF) {
                if (lex->current.type == TOK_COMMA) {
                    /* Elision */
                    lex_next(lex);
                    idx++;
                    continue;
                }
                JSValue elem = eval_assignment(ctx, lex);
                if (has_exception(ctx)) { JS_FreeValue(ctx, arr); return JS_EXCEPTION; }
                /* Grow array if needed */
                if (idx >= o->array_cap) {
                    int new_cap = o->array_cap * 2;
                    JSValue *new_data = (JSValue *)js_realloc(ctx->rt, o->array_data,
                        sizeof(JSValue) * o->array_cap, sizeof(JSValue) * new_cap);
                    if (!new_data) { JS_FreeValue(ctx, arr); return JS_EXCEPTION; }
                    o->array_data = new_data;
                    o->array_cap = new_cap;
                }
                o->array_data[idx] = elem;
                if (idx >= o->array_len) o->array_len = idx + 1;
                idx++;
                if (lex->current.type == TOK_COMMA) lex_next(lex);
            }
            if (lex->current.type == TOK_RBRACKET) lex_next(lex);
            return arr;
        }
        case TOK_LBRACE: {
            /* Object literal */
            lex_next(lex); /* skip { */
            JSValue obj = js_mkobject(ctx);
            JSObject_s *o = js_get_object(obj);
            while (lex->current.type != TOK_RBRACE &&
                   lex->current.type != TOK_EOF) {
                /* Property name */
                char *key = NULL;
                if (lex->current.type == TOK_STRING) {
                    key = js_strndup(ctx->rt, lex->current.start, lex->current.len);
                    lex_next(lex);
                } else if (lex->current.type == TOK_IDENT ||
                           lex->current.type >= TOK_VAR /* keywords as prop names */) {
                    key = js_strndup(ctx->rt, lex->current.start, lex->current.len);
                    lex_next(lex);
                } else if (lex->current.type == TOK_NUMBER) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%g", lex->current.num_val);
                    key = js_strdup(ctx->rt, buf);
                    lex_next(lex);
                } else {
                    break;
                }

                JSValue val;
                if (lex->current.type == TOK_COLON) {
                    lex_next(lex); /* skip : */
                    val = eval_assignment(ctx, lex);
                } else {
                    /* Shorthand: { key } means { key: key } */
                    val = scope_get(ctx->current_scope, key);
                }
                if (has_exception(ctx)) {
                    js_free(ctx->rt, key, strlen(key)+1);
                    JS_FreeValue(ctx, obj);
                    return JS_EXCEPTION;
                }
                obj_set_prop(ctx->rt, o, key, val);
                js_free(ctx->rt, key, strlen(key)+1);
                if (lex->current.type == TOK_COMMA) lex_next(lex);
            }
            if (lex->current.type == TOK_RBRACE) lex_next(lex);
            return obj;
        }
        case TOK_FUNCTION: {
            lex_next(lex); /* skip 'function' */
            char *fname = NULL;
            if (lex->current.type == TOK_IDENT) {
                fname = js_strndup(ctx->rt, lex->current.start, lex->current.len);
                lex_next(lex);
            }
            /* Parse parameter list */
            if (lex->current.type == TOK_LPAREN) lex_next(lex);
            char **params = NULL;
            int param_count = 0;
            int param_cap = 4;
            params = (char **)js_malloc(ctx->rt, sizeof(char *) * param_cap);
            while (lex->current.type != TOK_RPAREN &&
                   lex->current.type != TOK_EOF) {
                if (lex->current.type == TOK_IDENT) {
                    if (param_count >= param_cap) {
                        param_cap *= 2;
                        params = (char **)js_realloc(ctx->rt, params,
                            sizeof(char *) * param_count,
                            sizeof(char *) * param_cap);
                    }
                    params[param_count++] = js_strndup(ctx->rt,
                        lex->current.start, lex->current.len);
                }
                lex_next(lex);
                if (lex->current.type == TOK_COMMA) lex_next(lex);
            }
            if (lex->current.type == TOK_RPAREN) lex_next(lex);

            /* Collect function body */
            if (lex->current.type != TOK_LBRACE) {
                return throw_error(ctx, "expected { in function body");
            }
            (void)0; /* body_start calculated from body_begin below */
            int brace = 1;
            /* Skip the opening brace that lex_next already consumed */
            /* Actually current token IS { so we need to skip the body */
            int body_begin = lex->pos; /* position after { */
            lex_next(lex); /* consume { - but we need raw source */
            /* Re-scan from raw position, counting braces */
            int raw_pos = body_begin;
            char in_quote = 0;
            while (raw_pos < lex->len && brace > 0) {
                char c = lex->src[raw_pos];
                if (in_quote) {
                    if (c == in_quote && (raw_pos == 0 || lex->src[raw_pos-1] != '\\'))
                        in_quote = 0;
                } else {
                    if (c == '"' || c == '\'' || c == '`') in_quote = c;
                    else if (c == '{') brace++;
                    else if (c == '}') brace--;
                }
                if (brace > 0) raw_pos++;
            }
            size_t body_len = raw_pos - body_begin;
            /* Advance lexer past the body */
            lex->pos = raw_pos + 1; /* skip closing } */
            lex->has_peek = 0;
            lex_next(lex);

            JSValue func = js_mkfunc_js(ctx, lex->src + body_begin, body_len,
                                        params, param_count, ctx->current_scope);
            if (fname) {
                JSFunc_s *f = js_get_func(func);
                if (f) { js_free(ctx->rt, f->name, 1); f->name = fname; }
                /* Also define in current scope */
                scope_define(ctx->rt, ctx->current_scope, fname,
                            js_dup_value(func));
            }
            return func;
        }
        case TOK_NEW: {
            lex_next(lex); /* skip 'new' */
            /* Parse constructor call */
            JSValue ctor = eval_primary(ctx, lex, NULL);
            /* Parse arguments */
            JSValue args[16];
            int argc = 0;
            if (lex->current.type == TOK_LPAREN) {
                lex_next(lex);
                while (lex->current.type != TOK_RPAREN &&
                       lex->current.type != TOK_EOF && argc < 16) {
                    args[argc++] = eval_assignment(ctx, lex);
                    if (lex->current.type == TOK_COMMA) lex_next(lex);
                }
                if (lex->current.type == TOK_RPAREN) lex_next(lex);
            }
            /* Create new object */
            JSValue obj = js_mkobject(ctx);
            /* Call constructor with new object as 'this' */
            JSFunc_s *f = js_get_func(ctor);
            if (f && f->type == FUNC_C) {
                JSValue result = f->u.c.callback(ctx, obj, argc, args);
                for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                JS_FreeValue(ctx, ctor);
                if (JS_IsObject(result)) { JS_FreeValue(ctx, obj); return result; }
                JS_FreeValue(ctx, result);
                return obj;
            }
            for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
            JS_FreeValue(ctx, ctor);
            return obj;
        }
        case TOK_TYPEOF: {
            lex_next(lex); /* skip 'typeof' */
            JSValue val = eval_primary(ctx, lex, NULL);
            /* Clear exception for typeof on undeclared vars */
            if (has_exception(ctx)) {
                JS_FreeValue(ctx, ctx->current_exception);
                ctx->has_exception = 0;
                val = JS_UNDEFINED;
            }
            const char *type_str;
            int tag = JS_VALUE_GET_TAG(val);
            switch (tag) {
                case JS_TAG_UNDEFINED: type_str = "undefined"; break;
                case JS_TAG_NULL: type_str = "object"; break;
                case JS_TAG_BOOL: type_str = "boolean"; break;
                case JS_TAG_INT:
                case JS_TAG_FLOAT64: type_str = "number"; break;
                case JS_TAG_STRING: type_str = "string"; break;
                case JS_TAG_FUNCTION:
                case JS_TAG_C_FUNCTION: type_str = "function"; break;
                default: type_str = "object"; break;
            }
            JS_FreeValue(ctx, val);
            return js_mkstring(ctx, type_str, strlen(type_str));
        }
        case TOK_VOID: {
            lex_next(lex);
            JSValue val = eval_primary(ctx, lex, NULL);
            JS_FreeValue(ctx, val);
            return JS_UNDEFINED;
        }
        case TOK_NOT: {
            lex_next(lex);
            JSValue val = eval_primary(ctx, lex, NULL);
            int result = !js_is_truthy(val);
            JS_FreeValue(ctx, val);
            return result ? JS_TRUE : JS_FALSE;
        }
        case TOK_MINUS: {
            lex_next(lex);
            JSValue val = eval_primary(ctx, lex, NULL);
            double d = js_value_to_number(val);
            JS_FreeValue(ctx, val);
            return js_mkfloat(ctx, -d);
        }
        case TOK_PLUS: {
            lex_next(lex);
            JSValue val = eval_primary(ctx, lex, NULL);
            double d = js_value_to_number(val);
            JS_FreeValue(ctx, val);
            return js_mkfloat(ctx, d);
        }
        case TOK_BNOT: {
            lex_next(lex);
            JSValue val = eval_primary(ctx, lex, NULL);
            int32_t n = (int32_t)js_value_to_number(val);
            JS_FreeValue(ctx, val);
            return JS_MKVAL(JS_TAG_INT, (uint32_t)(~n));
        }
        case TOK_INC:
        case TOK_DEC: {
            int is_inc = (tok.type == TOK_INC);
            lex_next(lex);
            LValue lv2 = { JS_UNDEFINED, NULL, NULL };
            JSValue val = eval_primary(ctx, lex, &lv2);
            double d = js_value_to_number(val);
            JSValue new_val = js_mkfloat(ctx, is_inc ? d + 1 : d - 1);
            if (lv2.var_name) {
                scope_set(ctx->rt, ctx->current_scope, lv2.var_name,
                         js_dup_value(new_val));
            }
            JS_FreeValue(ctx, val);
            lvalue_clear(ctx, &lv2);
            return new_val;
        }
        case TOK_DELETE: {
            lex_next(lex);
            JSValue del_val = eval_primary(ctx, lex, NULL);
            JS_FreeValue(ctx, del_val);
            return JS_TRUE;
        }
        default:
            return throw_error(ctx, "unexpected token at line %d", tok.line);
    }
}


/* Postfix and member access */
static JSValue eval_postfix(JSContext *ctx, Lexer *lex, LValue *lv) {
    LValue inner_lv = { JS_UNDEFINED, NULL, NULL };
    JSValue val = eval_primary(ctx, lex, &inner_lv);
    if (has_exception(ctx)) return JS_EXCEPTION;

    for (;;) {
        if (lex->current.type == TOK_DOT || lex->current.type == TOK_OPTIONAL_CHAIN) {
            int is_optional = (lex->current.type == TOK_OPTIONAL_CHAIN);
            lex_next(lex); /* skip . or ?. */
            if (is_optional && (JS_IsNull(val) || JS_IsUndefined(val))) {
                lvalue_clear(ctx, &inner_lv);
                JS_FreeValue(ctx, val);
                val = JS_UNDEFINED;
                continue;
            }
            if (lex->current.type != TOK_IDENT &&
                lex->current.type < TOK_VAR) {
                return throw_error(ctx, "expected property name");
            }
            char *prop = js_strndup(ctx->rt, lex->current.start, lex->current.len);
            lex_next(lex);

            /* Get property from object */
            JSValue prop_val = JS_UNDEFINED;
            JSObject_s *o = js_get_object(val);
            if (o) {
                prop_val = obj_get_prop(o, prop);
            } else if (JS_IsString(val)) {
                /* String methods */
                JSString_s *s = js_get_string(val);
                if (strcmp(prop, "length") == 0) {
                    prop_val = JS_MKVAL(JS_TAG_INT, (uint32_t)s->len);
                } else if (strcmp(prop, "indexOf") == 0 ||
                           strcmp(prop, "substring") == 0 ||
                           strcmp(prop, "slice") == 0 ||
                           strcmp(prop, "replace") == 0 ||
                           strcmp(prop, "split") == 0 ||
                           strcmp(prop, "trim") == 0 ||
                           strcmp(prop, "toLowerCase") == 0 ||
                           strcmp(prop, "toUpperCase") == 0 ||
                           strcmp(prop, "charAt") == 0 ||
                           strcmp(prop, "charCodeAt") == 0 ||
                           strcmp(prop, "includes") == 0 ||
                           strcmp(prop, "startsWith") == 0 ||
                           strcmp(prop, "endsWith") == 0 ||
                           strcmp(prop, "match") == 0 ||
                           strcmp(prop, "concat") == 0) {
                    /* Create a bound string method marker */
                    /* We'll handle these in the call expression */
                    JSValue method_obj = js_mkobject(ctx);
                    JSObject_s *mo = js_get_object(method_obj);
                    obj_set_prop(ctx->rt, mo, "__string_method__",
                                js_mkstring(ctx, prop, strlen(prop)));
                    obj_set_prop(ctx->rt, mo, "__this__", js_dup_value(val));
                    prop_val = method_obj;
                }
            }

            lvalue_clear(ctx, &inner_lv);
            inner_lv.obj = js_dup_value(val);
            inner_lv.prop_name = prop;
            JS_FreeValue(ctx, val);
            val = prop_val;
            continue;
        }

        if (lex->current.type == TOK_LBRACKET) {
            lex_next(lex); /* skip [ */
            JSValue idx = eval_assignment(ctx, lex);
            if (lex->current.type == TOK_RBRACKET) lex_next(lex);

            JSValue prop_val = JS_UNDEFINED;
            JSObject_s *o = js_get_object(val);
            if (o) {
                int tag = JS_VALUE_GET_TAG(idx);
                if (tag == JS_TAG_INT) {
                    uint32_t i = (uint32_t)JS_VALUE_GET_INT(idx);
                    if (o->array_data && (int)i < o->array_len)
                        prop_val = js_dup_value(o->array_data[i]);
                    else
                        prop_val = obj_get_prop(o, ""); /* fallback */
                } else if (tag == JS_TAG_STRING) {
                    JSString_s *s = js_get_string(idx);
                    prop_val = obj_get_prop(o, s->data);
                } else {
                    JSValue sIdx = js_tostring(ctx, idx);
                    JSString_s *s = js_get_string(sIdx);
                    if (s) prop_val = obj_get_prop(o, s->data);
                    JS_FreeValue(ctx, sIdx);
                }
            } else if (JS_IsString(val)) {
                /* String indexing */
                JSString_s *s = js_get_string(val);
                if (JS_VALUE_GET_TAG(idx) == JS_TAG_INT) {
                    int i = JS_VALUE_GET_INT(idx);
                    if (s && i >= 0 && (size_t)i < s->len) {
                        prop_val = js_mkstring(ctx, s->data + i, 1);
                    }
                }
            }

            lvalue_clear(ctx, &inner_lv);
            /* Store lvalue info for bracket access */
            if (JS_VALUE_GET_TAG(idx) == JS_TAG_STRING) {
                JSString_s *s = js_get_string(idx);
                inner_lv.prop_name = js_strdup(ctx->rt, s->data);
            }
            inner_lv.obj = js_dup_value(val);
            JS_FreeValue(ctx, idx);
            JS_FreeValue(ctx, val);
            val = prop_val;
            continue;
        }

        if (lex->current.type == TOK_LPAREN) {
            /* Function call */
            lex_next(lex); /* skip ( */
            JSValue args[32];
            int argc = 0;
            while (lex->current.type != TOK_RPAREN &&
                   lex->current.type != TOK_EOF && argc < 32) {
                args[argc++] = eval_assignment(ctx, lex);
                if (has_exception(ctx)) {
                    for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    return JS_EXCEPTION;
                }
                if (lex->current.type == TOK_COMMA) lex_next(lex);
            }
            if (lex->current.type == TOK_RPAREN) lex_next(lex);

            /* Check for string methods */
            JSObject_s *method_obj = js_get_object(val);
            if (method_obj) {
                JSValue method_name = obj_get_prop(method_obj, "__string_method__");
                if (JS_IsString(method_name)) {
                    JSValue this_str = obj_get_prop(method_obj, "__this__");
                    JSString_s *sn = js_get_string(method_name);
                    JSString_s *ts = js_get_string(this_str);
                    JSValue result = JS_UNDEFINED;

                    if (ts && sn) {
                        if (strcmp(sn->data, "indexOf") == 0 && argc > 0 && JS_IsString(args[0])) {
                            JSString_s *needle = js_get_string(args[0]);
                            char *found = needle ? strstr(ts->data, needle->data) : NULL;
                            result = JS_MKVAL(JS_TAG_INT, found ? (uint32_t)(found - ts->data) : (uint32_t)-1);
                        } else if (strcmp(sn->data, "substring") == 0 || strcmp(sn->data, "slice") == 0) {
                            int start = 0, end = (int)ts->len;
                            if (argc > 0) start = (int)js_value_to_number(args[0]);
                            if (argc > 1) end = (int)js_value_to_number(args[1]);
                            if (start < 0) start = 0;
                            if (end > (int)ts->len) end = (int)ts->len;
                            if (start > end) { int t = start; start = end; end = t; }
                            result = js_mkstring(ctx, ts->data + start, end - start);
                        } else if (strcmp(sn->data, "trim") == 0) {
                            const char *s = ts->data;
                            int len = (int)ts->len;
                            while (len > 0 && isspace(s[0])) { s++; len--; }
                            while (len > 0 && isspace(s[len-1])) { len--; }
                            result = js_mkstring(ctx, s, len);
                        } else if (strcmp(sn->data, "toLowerCase") == 0) {
                            char *buf = js_strndup(ctx->rt, ts->data, ts->len);
                            for (size_t i = 0; i < ts->len; i++) buf[i] = tolower(buf[i]);
                            result = js_mkstring(ctx, buf, ts->len);
                            js_free(ctx->rt, buf, ts->len + 1);
                        } else if (strcmp(sn->data, "toUpperCase") == 0) {
                            char *buf = js_strndup(ctx->rt, ts->data, ts->len);
                            for (size_t i = 0; i < ts->len; i++) buf[i] = toupper(buf[i]);
                            result = js_mkstring(ctx, buf, ts->len);
                            js_free(ctx->rt, buf, ts->len + 1);
                        } else if (strcmp(sn->data, "split") == 0 && argc > 0) {
                            result = js_mkarray(ctx);
                            JSObject_s *arr = js_get_object(result);
                            JSString_s *sep = js_get_string(args[0]);
                            if (sep && sep->len > 0) {
                                const char *p = ts->data;
                                int idx = 0;
                                while (*p) {
                                    const char *found = strstr(p, sep->data);
                                    if (!found) {
                                        if (idx >= arr->array_cap) {
                                            int nc = arr->array_cap * 2;
                                            arr->array_data = (JSValue*)js_realloc(ctx->rt, arr->array_data,
                                                sizeof(JSValue)*arr->array_cap, sizeof(JSValue)*nc);
                                            arr->array_cap = nc;
                                        }
                                        arr->array_data[idx] = js_mkstring(ctx, p, strlen(p));
                                        arr->array_len = idx + 1;
                                        break;
                                    }
                                    if (idx >= arr->array_cap) {
                                        int nc = arr->array_cap * 2;
                                        arr->array_data = (JSValue*)js_realloc(ctx->rt, arr->array_data,
                                            sizeof(JSValue)*arr->array_cap, sizeof(JSValue)*nc);
                                        arr->array_cap = nc;
                                    }
                                    arr->array_data[idx] = js_mkstring(ctx, p, found - p);
                                    arr->array_len = idx + 1;
                                    idx++;
                                    p = found + sep->len;
                                }
                            }
                        } else if (strcmp(sn->data, "charAt") == 0 && argc > 0) {
                            int i = (int)js_value_to_number(args[0]);
                            if (i >= 0 && (size_t)i < ts->len)
                                result = js_mkstring(ctx, ts->data + i, 1);
                            else
                                result = js_mkstring(ctx, "", 0);
                        } else if (strcmp(sn->data, "includes") == 0 && argc > 0 && JS_IsString(args[0])) {
                            JSString_s *needle = js_get_string(args[0]);
                            result = (needle && strstr(ts->data, needle->data)) ? JS_TRUE : JS_FALSE;
                        } else if (strcmp(sn->data, "startsWith") == 0 && argc > 0 && JS_IsString(args[0])) {
                            JSString_s *pfx = js_get_string(args[0]);
                            result = (pfx && strncmp(ts->data, pfx->data, pfx->len) == 0) ? JS_TRUE : JS_FALSE;
                        } else if (strcmp(sn->data, "endsWith") == 0 && argc > 0 && JS_IsString(args[0])) {
                            JSString_s *sfx = js_get_string(args[0]);
                            result = (sfx && ts->len >= sfx->len &&
                                     strcmp(ts->data + ts->len - sfx->len, sfx->data) == 0) ? JS_TRUE : JS_FALSE;
                        } else if (strcmp(sn->data, "replace") == 0 && argc >= 2 && JS_IsString(args[0])) {
                            JSString_s *pattern = js_get_string(args[0]);
                            JSValue rep_str = js_tostring(ctx, args[1]);
                            JSString_s *rep = js_get_string(rep_str);
                            if (pattern && rep) {
                                char *found = strstr(ts->data, pattern->data);
                                if (found) {
                                    size_t new_len = ts->len - pattern->len + rep->len;
                                    char *buf = (char*)js_malloc(ctx->rt, new_len + 1);
                                    size_t prefix_len = found - ts->data;
                                    memcpy(buf, ts->data, prefix_len);
                                    memcpy(buf + prefix_len, rep->data, rep->len);
                                    memcpy(buf + prefix_len + rep->len,
                                           found + pattern->len,
                                           ts->len - prefix_len - pattern->len);
                                    buf[new_len] = '\0';
                                    result = js_mkstring(ctx, buf, new_len);
                                    js_free(ctx->rt, buf, new_len + 1);
                                } else {
                                    result = js_dup_value(this_str);
                                }
                            }
                            JS_FreeValue(ctx, rep_str);
                        } else if (strcmp(sn->data, "concat") == 0 && argc > 0) {
                            result = js_dup_value(this_str);
                            for (int i = 0; i < argc; i++) {
                                JSValue tmp = js_string_concat(ctx, result, args[i]);
                                JS_FreeValue(ctx, result);
                                result = tmp;
                            }
                        } else if (strcmp(sn->data, "charCodeAt") == 0 && argc > 0) {
                            int i = (int)js_value_to_number(args[0]);
                            if (i >= 0 && (size_t)i < ts->len)
                                result = JS_MKVAL(JS_TAG_INT, (uint32_t)(unsigned char)ts->data[i]);
                            else
                                result = js_mkfloat(ctx, NAN);
                        }
                    }

                    JS_FreeValue(ctx, method_name);
                    JS_FreeValue(ctx, this_str);
                    for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    val = result;
                    inner_lv.obj = JS_UNDEFINED;
                    inner_lv.prop_name = NULL;
                    inner_lv.var_name = NULL;
                    continue;
                }
                JS_FreeValue(ctx, method_name);
            }

            /* Check for array methods */
            int parent_tag = JS_VALUE_GET_TAG(inner_lv.obj);
            if (parent_tag == JS_TAG_ARRAY && inner_lv.prop_name) {
                JSObject_s *arr = js_get_object(inner_lv.obj);
                if (arr && strcmp(inner_lv.prop_name, "push") == 0) {
                    for (int i = 0; i < argc; i++) {
                        if (arr->array_len >= arr->array_cap) {
                            int nc = arr->array_cap * 2;
                            arr->array_data = (JSValue*)js_realloc(ctx->rt,
                                arr->array_data, sizeof(JSValue)*arr->array_cap,
                                sizeof(JSValue)*nc);
                            arr->array_cap = nc;
                        }
                        arr->array_data[arr->array_len++] = args[i];
                    }
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    val = JS_MKVAL(JS_TAG_INT, (uint32_t)arr->array_len);
                    inner_lv.obj = JS_UNDEFINED; inner_lv.prop_name = NULL;
                    continue;
                } else if (arr && strcmp(inner_lv.prop_name, "pop") == 0) {
                    JSValue result = JS_UNDEFINED;
                    if (arr->array_len > 0) {
                        arr->array_len--;
                        result = arr->array_data[arr->array_len];
                    }
                    for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    val = result;
                    inner_lv.obj = JS_UNDEFINED; inner_lv.prop_name = NULL;
                    continue;
                } else if (arr && strcmp(inner_lv.prop_name, "join") == 0) {
                    const char *sep = ",";
                    (void)0;
                    if (argc > 0 && JS_IsString(args[0])) {
                        JSString_s *ss = js_get_string(args[0]);
                        sep = ss->data;
                    }
                    /* Join array elements */
                    size_t total = 0;
                    size_t sep_len = strlen(sep);
                    for (int i = 0; i < arr->array_len; i++) {
                        JSValue s = js_tostring(ctx, arr->array_data[i]);
                        JSString_s *ss = js_get_string(s);
                        if (ss) total += ss->len;
                        if (i > 0) total += sep_len;
                        JS_FreeValue(ctx, s);
                    }
                    char *buf = (char*)js_malloc(ctx->rt, total + 1);
                    buf[0] = '\0';
                    size_t pos = 0;
                    for (int i = 0; i < arr->array_len; i++) {
                        if (i > 0) { memcpy(buf+pos, sep, sep_len); pos += sep_len; }
                        JSValue s = js_tostring(ctx, arr->array_data[i]);
                        JSString_s *ss = js_get_string(s);
                        if (ss) { memcpy(buf+pos, ss->data, ss->len); pos += ss->len; }
                        JS_FreeValue(ctx, s);
                    }
                    buf[pos] = '\0';
                    JSValue result = js_mkstring(ctx, buf, pos);
                    js_free(ctx->rt, buf, total + 1);
                    for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    val = result;
                    inner_lv.obj = JS_UNDEFINED; inner_lv.prop_name = NULL;
                    continue;
                } else if (arr && strcmp(inner_lv.prop_name, "forEach") == 0 && argc > 0) {
                    JSFunc_s *cb = js_get_func(args[0]);
                    if (cb) {
                        for (int i = 0; i < arr->array_len; i++) {
                            JSValue cb_args[3];
                            cb_args[0] = js_dup_value(arr->array_data[i]);
                            cb_args[1] = JS_MKVAL(JS_TAG_INT, (uint32_t)i);
                            cb_args[2] = js_dup_value(inner_lv.obj);
                            if (cb->type == FUNC_C) {
                                JSValue r = cb->u.c.callback(ctx, JS_UNDEFINED, 3, cb_args);
                                JS_FreeValue(ctx, r);
                            }
                            /* TODO: handle JS functions */
                            for (int j = 0; j < 3; j++) JS_FreeValue(ctx, cb_args[j]);
                        }
                    }
                    for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                    JS_FreeValue(ctx, val);
                    lvalue_clear(ctx, &inner_lv);
                    val = JS_UNDEFINED;
                    inner_lv.obj = JS_UNDEFINED; inner_lv.prop_name = NULL;
                    continue;
                }
            }

            /* Regular function call */
            JSFunc_s *f = js_get_func(val);
            if (!f) {
                /* Not a function - skip */
                for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
                JS_FreeValue(ctx, val);
                lvalue_clear(ctx, &inner_lv);
                val = JS_UNDEFINED;
                inner_lv.obj = JS_UNDEFINED; inner_lv.prop_name = NULL;
                continue;
            }

            ctx->stats.function_calls++;
            JSValue this_val = JS_IsUndefined(inner_lv.obj) ?
                               ctx->global_obj : inner_lv.obj;
            JSValue result;

            if (f->type == FUNC_C) {
                result = f->u.c.callback(ctx, this_val, argc, args);
            } else {
                /* JS function call */
                Scope *call_scope = scope_new(ctx->rt,
                    f->u.js.closure ? f->u.js.closure : ctx->current_scope);
                if (!call_scope) {
                    result = JS_EXCEPTION;
                } else {
                    /* Bind parameters */
                    for (int i = 0; i < f->u.js.param_count; i++) {
                        JSValue arg = (i < argc) ? js_dup_value(args[i]) : JS_UNDEFINED;
                        scope_define(ctx->rt, call_scope, f->u.js.params[i], arg);
                    }
                    /* Define 'arguments' */
                    JSValue args_arr = js_mkarray(ctx);
                    JSObject_s *args_obj = js_get_object(args_arr);
                    for (int i = 0; i < argc; i++) {
                        if (i >= args_obj->array_cap) {
                            int nc = args_obj->array_cap * 2;
                            args_obj->array_data = (JSValue*)js_realloc(ctx->rt,
                                args_obj->array_data, sizeof(JSValue)*args_obj->array_cap,
                                sizeof(JSValue)*nc);
                            args_obj->array_cap = nc;
                        }
                        args_obj->array_data[i] = js_dup_value(args[i]);
                        args_obj->array_len = i + 1;
                    }
                    scope_define(ctx->rt, call_scope, "arguments", args_arr);

                    /* Execute function body */
                    Scope *saved_scope = ctx->current_scope;
                    ctx->current_scope = call_scope;
                    ctx->scope_depth++;

                    if (ctx->scope_depth > MAX_SCOPE_DEPTH) {
                        result = throw_error(ctx, "Maximum call stack size exceeded");
                    } else {
                        result = js_eval_source(ctx, f->u.js.source,
                                               f->u.js.source_len);
                        /* Check if function returned via ctx->has_return */
                        if (ctx->has_return) {
                            JS_FreeValue(ctx, result);
                            result = ctx->return_value; /* take ownership */
                            ctx->return_value = JS_UNDEFINED;
                            ctx->has_return = 0;
                        }
                    }

                    ctx->scope_depth--;
                    ctx->current_scope = saved_scope;
                    scope_free(ctx->rt, call_scope);
                }
            }

            for (int i = 0; i < argc; i++) JS_FreeValue(ctx, args[i]);
            JS_FreeValue(ctx, val);
            lvalue_clear(ctx, &inner_lv);
            val = result;
            inner_lv.obj = JS_UNDEFINED;
            inner_lv.prop_name = NULL;
            inner_lv.var_name = NULL;
            continue;
        }

        /* Postfix ++ / -- */
        if (lex->current.type == TOK_INC || lex->current.type == TOK_DEC) {
            int is_inc = (lex->current.type == TOK_INC);
            lex_next(lex);
            double d = js_value_to_number(val);
            JSValue new_val = js_mkfloat(ctx, is_inc ? d + 1 : d - 1);
            if (inner_lv.var_name) {
                scope_set(ctx->rt, ctx->current_scope, inner_lv.var_name,
                         js_dup_value(new_val));
            } else if (inner_lv.prop_name && !JS_IsUndefined(inner_lv.obj)) {
                JSObject_s *o = js_get_object(inner_lv.obj);
                if (o) obj_set_prop(ctx->rt, o, inner_lv.prop_name,
                                   js_dup_value(new_val));
            }
            JS_FreeValue(ctx, new_val);
            /* Return old value */
            continue;
        }

        break; /* No more postfix operations */
    }

    /* Copy lvalue info if requested */
    if (lv) {
        *lv = inner_lv;
    } else {
        lvalue_clear(ctx, &inner_lv);
    }
    return val;
}


/* Binary expression with precedence climbing */
static JSValue eval_expression(JSContext *ctx, Lexer *lex, int min_prec) {
    if (has_exception(ctx)) return JS_EXCEPTION;

    JSValue left = eval_postfix(ctx, lex, NULL);
    if (has_exception(ctx)) return JS_EXCEPTION;

    for (;;) {
        TokenType op = lex->current.type;
        int prec = 0;
        int right_assoc = 0;

        /* Precedence table */
        switch (op) {
            case TOK_OR:      prec = 4; break;
            case TOK_NULLISH: prec = 4; break;
            case TOK_AND:     prec = 5; break;
            case TOK_BOR:     prec = 6; break;
            case TOK_BXOR:    prec = 7; break;
            case TOK_BAND:    prec = 8; break;
            case TOK_EQ: case TOK_NEQ: case TOK_SEQ: case TOK_SNEQ:
                prec = 9; break;
            case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE:
            case TOK_INSTANCEOF: case TOK_IN:
                prec = 10; break;
            case TOK_SHL: case TOK_SHR: case TOK_USHR:
                prec = 11; break;
            case TOK_PLUS: case TOK_MINUS:
                prec = 12; break;
            case TOK_STAR: case TOK_SLASH: case TOK_PERCENT:
                prec = 13; break;
            case TOK_POWER:
                prec = 14; right_assoc = 1; break;
            default:
                return left;
        }

        if (prec < min_prec) return left;

        lex_next(lex); /* consume operator */

        /* Short-circuit for && and || */
        if (op == TOK_AND) {
            if (!js_is_truthy(left)) {
                /* Short-circuit: consume right operand but keep left value */
                JSValue rhs = eval_expression(ctx, lex, prec + 1);
                JS_FreeValue(ctx, rhs);
                continue;
            }
            JS_FreeValue(ctx, left);
            left = eval_expression(ctx, lex, prec + 1);
            continue;
        }
        if (op == TOK_OR) {
            if (js_is_truthy(left)) {
                JSValue rhs = eval_expression(ctx, lex, prec + 1);
                JS_FreeValue(ctx, rhs);
                continue;
            }
            JS_FreeValue(ctx, left);
            left = eval_expression(ctx, lex, prec + 1);
            continue;
        }
        if (op == TOK_NULLISH) {
            if (!JS_IsNull(left) && !JS_IsUndefined(left)) {
                JSValue rhs = eval_expression(ctx, lex, prec + 1);
                JS_FreeValue(ctx, rhs);
                continue;
            }
            JS_FreeValue(ctx, left);
            left = eval_expression(ctx, lex, prec + 1);
            continue;
        }

        JSValue right = eval_expression(ctx, lex, right_assoc ? prec : prec + 1);
        if (has_exception(ctx)) { JS_FreeValue(ctx, left); return JS_EXCEPTION; }

        /* String concatenation if either operand is string and op is + */
        if (op == TOK_PLUS &&
            (JS_IsString(left) || JS_IsString(right))) {
            JSValue result = js_string_concat(ctx, left, right);
            JS_FreeValue(ctx, left);
            JS_FreeValue(ctx, right);
            left = result;
            continue;
        }

        /* Arithmetic / comparison */
        double ld = js_value_to_number(left);
        double rd = js_value_to_number(right);
        JSValue result;

        switch (op) {
            case TOK_PLUS:    result = js_mkfloat(ctx, ld + rd); break;
            case TOK_MINUS:   result = js_mkfloat(ctx, ld - rd); break;
            case TOK_STAR:    result = js_mkfloat(ctx, ld * rd); break;
            case TOK_SLASH:   result = js_mkfloat(ctx, rd != 0 ? ld / rd : NAN); break;
            case TOK_PERCENT: result = js_mkfloat(ctx, rd != 0 ? fmod(ld, rd) : NAN); break;
            case TOK_POWER:   result = js_mkfloat(ctx, pow(ld, rd)); break;
            case TOK_LT:  result = (ld < rd) ? JS_TRUE : JS_FALSE; break;
            case TOK_GT:  result = (ld > rd) ? JS_TRUE : JS_FALSE; break;
            case TOK_LE:  result = (ld <= rd) ? JS_TRUE : JS_FALSE; break;
            case TOK_GE:  result = (ld >= rd) ? JS_TRUE : JS_FALSE; break;
            case TOK_EQ:  result = js_abstract_eq(left, right) ? JS_TRUE : JS_FALSE; break;
            case TOK_NEQ: result = js_abstract_eq(left, right) ? JS_FALSE : JS_TRUE; break;
            case TOK_SEQ: result = js_strict_eq(left, right) ? JS_TRUE : JS_FALSE; break;
            case TOK_SNEQ: result = js_strict_eq(left, right) ? JS_FALSE : JS_TRUE; break;
            case TOK_SHL:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld << (int32_t)rd)); break;
            case TOK_SHR:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld >> (int32_t)rd)); break;
            case TOK_USHR: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((uint32_t)ld >> (uint32_t)rd)); break;
            case TOK_BOR:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld | (int32_t)rd)); break;
            case TOK_BAND: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld & (int32_t)rd)); break;
            case TOK_BXOR: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld ^ (int32_t)rd)); break;
            case TOK_INSTANCEOF: result = JS_FALSE; break; /* simplified */
            case TOK_IN: result = JS_FALSE; break; /* simplified */
            default: result = JS_UNDEFINED; break;
        }

        JS_FreeValue(ctx, left);
        JS_FreeValue(ctx, right);
        left = result;
    }
}

/* Assignment expression */
static JSValue eval_assignment(JSContext *ctx, Lexer *lex) {
    if (has_exception(ctx)) return JS_EXCEPTION;

    LValue lv = { JS_UNDEFINED, NULL, NULL };
    JSValue left = eval_postfix(ctx, lex, &lv);
    if (has_exception(ctx)) {
        lvalue_clear(ctx, &lv);
        return JS_EXCEPTION;
    }

    TokenType op = lex->current.type;
    if (op == TOK_ASSIGN || op == TOK_PLUS_ASSIGN || op == TOK_MINUS_ASSIGN ||
        op == TOK_STAR_ASSIGN || op == TOK_SLASH_ASSIGN || op == TOK_PERCENT_ASSIGN) {
        lex_next(lex);
        JSValue right = eval_assignment(ctx, lex);
        if (has_exception(ctx)) {
            JS_FreeValue(ctx, left);
            lvalue_clear(ctx, &lv);
            return JS_EXCEPTION;
        }

        JSValue new_val;
        if (op == TOK_ASSIGN) {
            new_val = right;
        } else {
            /* Compound assignment */
            if (op == TOK_PLUS_ASSIGN && (JS_IsString(left) || JS_IsString(right))) {
                new_val = js_string_concat(ctx, left, right);
                JS_FreeValue(ctx, right);
            } else {
                double ld = js_value_to_number(left);
                double rd = js_value_to_number(right);
                double result;
                switch (op) {
                    case TOK_PLUS_ASSIGN:    result = ld + rd; break;
                    case TOK_MINUS_ASSIGN:   result = ld - rd; break;
                    case TOK_STAR_ASSIGN:    result = ld * rd; break;
                    case TOK_SLASH_ASSIGN:   result = rd != 0 ? ld / rd : NAN; break;
                    case TOK_PERCENT_ASSIGN: result = rd != 0 ? fmod(ld, rd) : NAN; break;
                    default: result = rd; break;
                }
                JS_FreeValue(ctx, right);
                new_val = js_mkfloat(ctx, result);
            }
        }

        /* Store the value */
        if (lv.var_name) {
            scope_set(ctx->rt, ctx->current_scope, lv.var_name,
                     js_dup_value(new_val));
        } else if (lv.prop_name && !JS_IsUndefined(lv.obj)) {
            JSObject_s *o = js_get_object(lv.obj);
            if (o) obj_set_prop(ctx->rt, o, lv.prop_name,
                               js_dup_value(new_val));
        }

        JS_FreeValue(ctx, left);
        lvalue_clear(ctx, &lv);
        return new_val;
    }

    /* Not an assignment — continue with ternary */
    lvalue_clear(ctx, &lv);

    /* Check for ternary */
    if (lex->current.type == TOK_QUESTION) {
        lex_next(lex);
        int cond = js_is_truthy(left);
        JS_FreeValue(ctx, left);

        if (cond) {
            left = eval_assignment(ctx, lex);
            if (lex->current.type == TOK_COLON) {
                lex_next(lex);
                skip_expression(lex); /* skip false branch without evaluating */
            }
        } else {
            skip_expression(lex); /* skip true branch without evaluating */
            if (lex->current.type == TOK_COLON) {
                lex_next(lex);
                left = eval_assignment(ctx, lex);
            } else {
                left = JS_UNDEFINED;
            }
        }
        return left;
    }

    /* Check for binary operators after the postfix expression */
    /* We need to feed this into the precedence climbing parser */
    /* Save the left value and continue with binary operations */
    TokenType next_op = lex->current.type;
    int has_binop = 0;
    switch (next_op) {
        case TOK_PLUS: case TOK_MINUS: case TOK_STAR: case TOK_SLASH:
        case TOK_PERCENT: case TOK_POWER:
        case TOK_EQ: case TOK_NEQ: case TOK_SEQ: case TOK_SNEQ:
        case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE:
        case TOK_AND: case TOK_OR: case TOK_NULLISH:
        case TOK_BAND: case TOK_BOR: case TOK_BXOR:
        case TOK_SHL: case TOK_SHR: case TOK_USHR:
        case TOK_INSTANCEOF: case TOK_IN:
            has_binop = 1;
            break;
        default:
            break;
    }

    if (has_binop) {
        /* Continue with binary expression parsing */
        /* We already have the left side from eval_postfix, now handle the operator */
        for (;;) {
            TokenType op2 = lex->current.type;
            int prec = 0;
            switch (op2) {
                case TOK_OR: case TOK_NULLISH: prec = 4; break;
                case TOK_AND: prec = 5; break;
                case TOK_BOR: prec = 6; break;
                case TOK_BXOR: prec = 7; break;
                case TOK_BAND: prec = 8; break;
                case TOK_EQ: case TOK_NEQ: case TOK_SEQ: case TOK_SNEQ: prec = 9; break;
                case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE:
                case TOK_INSTANCEOF: case TOK_IN: prec = 10; break;
                case TOK_SHL: case TOK_SHR: case TOK_USHR: prec = 11; break;
                case TOK_PLUS: case TOK_MINUS: prec = 12; break;
                case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: prec = 13; break;
                case TOK_POWER: prec = 14; break;
                default: prec = 0; break;
            }
            if (prec < 1) break;  /* exit binary loop, check ternary below */

            lex_next(lex);

            /* Short-circuit (consume right operand, then break for ternary check) */
            if (op2 == TOK_AND) {
                if (!js_is_truthy(left)) {
                    JSValue rhs = eval_expression(ctx, lex, prec + 1);
                    JS_FreeValue(ctx, rhs);
                    break; /* ternary may follow */
                }
                JS_FreeValue(ctx, left);
                left = eval_expression(ctx, lex, prec + 1);
                continue;
            }
            if (op2 == TOK_OR) {
                if (js_is_truthy(left)) {
                    JSValue rhs = eval_expression(ctx, lex, prec + 1);
                    JS_FreeValue(ctx, rhs);
                    break;
                }
                JS_FreeValue(ctx, left);
                left = eval_expression(ctx, lex, prec + 1);
                continue;
            }
            if (op2 == TOK_NULLISH) {
                if (!JS_IsNull(left) && !JS_IsUndefined(left)) {
                    JSValue rhs = eval_expression(ctx, lex, prec + 1);
                    JS_FreeValue(ctx, rhs);
                    break;
                }
                JS_FreeValue(ctx, left);
                left = eval_expression(ctx, lex, prec + 1);
                continue;
            }

            int right_assoc = (op2 == TOK_POWER) ? 1 : 0;
            JSValue right = eval_expression(ctx, lex, right_assoc ? prec : prec + 1);

            if (op2 == TOK_PLUS && (JS_IsString(left) || JS_IsString(right))) {
                JSValue r = js_string_concat(ctx, left, right);
                JS_FreeValue(ctx, left); JS_FreeValue(ctx, right);
                left = r;
                continue;
            }

            double ld = js_value_to_number(left);
            double rd = js_value_to_number(right);
            JSValue result;
            switch (op2) {
                case TOK_PLUS:    result = js_mkfloat(ctx, ld + rd); break;
                case TOK_MINUS:   result = js_mkfloat(ctx, ld - rd); break;
                case TOK_STAR:    result = js_mkfloat(ctx, ld * rd); break;
                case TOK_SLASH:   result = js_mkfloat(ctx, rd != 0 ? ld / rd : NAN); break;
                case TOK_PERCENT: result = js_mkfloat(ctx, rd != 0 ? fmod(ld, rd) : NAN); break;
                case TOK_POWER:   result = js_mkfloat(ctx, pow(ld, rd)); break;
                case TOK_LT:  result = (ld < rd) ? JS_TRUE : JS_FALSE; break;
                case TOK_GT:  result = (ld > rd) ? JS_TRUE : JS_FALSE; break;
                case TOK_LE:  result = (ld <= rd) ? JS_TRUE : JS_FALSE; break;
                case TOK_GE:  result = (ld >= rd) ? JS_TRUE : JS_FALSE; break;
                case TOK_EQ:  result = js_abstract_eq(left, right) ? JS_TRUE : JS_FALSE; break;
                case TOK_NEQ: result = !js_abstract_eq(left, right) ? JS_TRUE : JS_FALSE; break;
                case TOK_SEQ: result = js_strict_eq(left, right) ? JS_TRUE : JS_FALSE; break;
                case TOK_SNEQ: result = !js_strict_eq(left, right) ? JS_TRUE : JS_FALSE; break;
                case TOK_SHL:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld << (int32_t)rd)); break;
                case TOK_SHR:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld >> (int32_t)rd)); break;
                case TOK_USHR: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((uint32_t)ld >> (uint32_t)rd)); break;
                case TOK_BOR:  result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld | (int32_t)rd)); break;
                case TOK_BAND: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld & (int32_t)rd)); break;
                case TOK_BXOR: result = JS_MKVAL(JS_TAG_INT, (uint32_t)((int32_t)ld ^ (int32_t)rd)); break;
                default: result = JS_UNDEFINED; break;
            }
            JS_FreeValue(ctx, left); JS_FreeValue(ctx, right);
            left = result;
        }
    }

    /* Check for ternary after binary expression (e.g. a > b ? x : y) */
    if (lex->current.type == TOK_QUESTION) {
        lex_next(lex);
        int cond = js_is_truthy(left);
        JS_FreeValue(ctx, left);

        if (cond) {
            left = eval_assignment(ctx, lex);
            if (lex->current.type == TOK_COLON) {
                lex_next(lex);
                skip_expression(lex); /* skip false branch without evaluating */
            }
        } else {
            skip_expression(lex); /* skip true branch without evaluating */
            if (lex->current.type == TOK_COLON) {
                lex_next(lex);
                left = eval_assignment(ctx, lex);
            } else {
                left = JS_UNDEFINED;
            }
        }
    }

    return left;
}

/* eval_comma_expression: wraps eval_assignment with comma operator handling.
 * Used in expression statements where comma is an operator (e.g., "a = 1, b = 2;").
 * NOT used in function call arguments, array literals, etc. where comma is a separator. */
static JSValue eval_comma_expression(JSContext *ctx, Lexer *lex) {
    JSValue left = eval_assignment(ctx, lex);
    while (lex->current.type == TOK_COMMA) {
        lex_next(lex);
        JS_FreeValue(ctx, left);
        left = eval_assignment(ctx, lex);
    }
    return left;
}



/* ================================================================
 * Section 10: Statement Parser/Evaluator
 * ================================================================ */

static JSValue eval_statement(JSContext *ctx, Lexer *lex) {
    if (has_exception(ctx)) return JS_EXCEPTION;
    if (check_interrupt(ctx)) return JS_EXCEPTION;
    ctx->stats.statements_executed++;

    TokenType tok = lex->current.type;

    /* Empty statement */
    if (tok == TOK_SEMICOLON) {
        lex_next(lex);
        return JS_UNDEFINED;
    }

    /* Block */
    if (tok == TOK_LBRACE) {
        return eval_block(ctx, lex);
    }

    /* Variable declarations */
    if (tok == TOK_VAR || tok == TOK_LET || tok == TOK_CONST) {
        lex_next(lex);
        do {
            if (lex->current.type != TOK_IDENT) {
                /* Destructuring or error - skip */
                if (lex->current.type == TOK_LBRACE || lex->current.type == TOK_LBRACKET) {
                    /* Skip destructuring pattern */
                    int depth = 1;
                    lex_next(lex);
                    while (!lex_eof(lex) && depth > 0) {
                        if (lex->current.type == TOK_LBRACE || lex->current.type == TOK_LBRACKET) depth++;
                        else if (lex->current.type == TOK_RBRACE || lex->current.type == TOK_RBRACKET) depth--;
                        lex_next(lex);
                    }
                    if (lex->current.type == TOK_ASSIGN) {
                        lex_next(lex);
                        JSValue v = eval_assignment(ctx, lex);
                        JS_FreeValue(ctx, v);
                    }
                } else {
                    break;
                }
            } else {
                char *name = js_strndup(ctx->rt, lex->current.start, lex->current.len);
                lex_next(lex);
                JSValue init = JS_UNDEFINED;
                if (lex->current.type == TOK_ASSIGN) {
                    lex_next(lex);
                    init = eval_assignment(ctx, lex);
                    if (has_exception(ctx)) {
                        js_free(ctx->rt, name, strlen(name)+1);
                        return JS_EXCEPTION;
                    }
                }
                scope_define(ctx->rt, ctx->current_scope, name, init);
                js_free(ctx->rt, name, strlen(name)+1);
            }
        } while (lex->current.type == TOK_COMMA && (lex_next(lex), 1));
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return JS_UNDEFINED;
    }

    /* Function declaration */
    if (tok == TOK_FUNCTION) {
        JSValue func = eval_primary(ctx, lex, NULL);
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return func;
    }

    /* Return */
    if (tok == TOK_RETURN) {
        lex_next(lex);
        JSValue val = JS_UNDEFINED;
        if (lex->current.type != TOK_SEMICOLON &&
            lex->current.type != TOK_RBRACE &&
            lex->current.type != TOK_EOF) {
            val = eval_assignment(ctx, lex);
        }
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        /* Free previous return value if one was set (e.g. nested return) */
        if (ctx->has_return) {
            JS_FreeValue(ctx, ctx->return_value);
        }
        ctx->return_value = js_dup_value(val);
        ctx->has_return = 1;
        return val;
    }

    /* If/else */
    if (tok == TOK_IF) {
        lex_next(lex); /* skip 'if' */
        if (lex->current.type == TOK_LPAREN) lex_next(lex);
        JSValue cond = eval_assignment(ctx, lex);
        if (lex->current.type == TOK_RPAREN) lex_next(lex);

        int is_true = js_is_truthy(cond);
        JS_FreeValue(ctx, cond);

        JSValue result = JS_UNDEFINED;
        if (is_true) {
            result = eval_statement(ctx, lex);
            /* Skip else branch without evaluating */
            if (lex->current.type == TOK_ELSE) {
                lex_next(lex);
                skip_statement(lex);
            }
        } else {
            /* Skip true branch without evaluating */
            skip_statement(lex);
            /* Execute else branch */
            if (lex->current.type == TOK_ELSE) {
                lex_next(lex);
                result = eval_statement(ctx, lex);
            }
        }
        return result;
    }

    /* While loop */
    if (tok == TOK_WHILE) {
        lex_next(lex); /* skip 'while' */
        if (lex->current.type != TOK_LPAREN) return throw_error(ctx, "expected (");
        /* Save position before condition for loop */
        int cond_pos = lex->pos;
        int cond_line = lex->line;

        for (int iter = 0; iter < 100000; iter++) {
            /* Reset to condition */
            lex->pos = cond_pos;
            lex->line = cond_line;
            lex->has_peek = 0;
            lex_next(lex); /* get ( */
            if (lex->current.type == TOK_LPAREN) lex_next(lex);
            JSValue cond = eval_assignment(ctx, lex);
            if (has_exception(ctx)) return JS_EXCEPTION;
            if (lex->current.type == TOK_RPAREN) lex_next(lex);

            if (!js_is_truthy(cond)) {
                JS_FreeValue(ctx, cond);
                /* Skip body */
                if (lex->current.type == TOK_LBRACE) {
                    int depth = 1;
                    lex_next(lex);
                    while (!lex_eof(lex) && depth > 0) {
                        if (lex->current.type == TOK_LBRACE) depth++;
                        else if (lex->current.type == TOK_RBRACE) depth--;
                        if (depth > 0) lex_next(lex);
                    }
                    if (lex->current.type == TOK_RBRACE) lex_next(lex);
                }
                break;
            }
            JS_FreeValue(ctx, cond);

            JSValue body_result = eval_statement(ctx, lex);
            int is_break = JS_IS_BREAK(body_result);
            int is_continue = JS_IS_CONTINUE(body_result);
            int is_return = ctx->has_return;
            if (!is_break && !is_continue) JS_FreeValue(ctx, body_result);
            if (is_break || is_return || has_exception(ctx)) break;
            /* We'll reset to cond_pos at top of loop */
        }
        return JS_UNDEFINED;
    }

    /* For loop */
    if (tok == TOK_FOR) {
        lex_next(lex); /* skip 'for' */
        if (lex->current.type == TOK_LPAREN) lex_next(lex);

        /* Init */
        if (lex->current.type != TOK_SEMICOLON) {
            if (lex->current.type == TOK_VAR || lex->current.type == TOK_LET ||
                lex->current.type == TOK_CONST) {
                JSValue init = eval_statement(ctx, lex);
                JS_FreeValue(ctx, init);
            } else {
                JSValue init = eval_comma_expression(ctx, lex);
                JS_FreeValue(ctx, init);
                if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
            }
        } else {
            lex_next(lex); /* skip ; */
        }

        /* Save position before condition */
        int cond_pos = lex->pos;
        int cond_line = lex->line;
        Token cond_tok = lex->current;

        /* First pass: find the body and increment positions */
        /* Parse condition */
        int has_cond = (lex->current.type != TOK_SEMICOLON);
        JSValue first_cond = JS_TRUE;
        if (has_cond) first_cond = eval_assignment(ctx, lex);
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);

        /* Save increment position */
        int inc_pos = lex->pos;
        int inc_line = lex->line;
        Token inc_tok = lex->current;

        /* Skip increment */
        if (lex->current.type != TOK_RPAREN) {
            int depth = 0;
            while (!lex_eof(lex)) {
                if (lex->current.type == TOK_LPAREN) depth++;
                else if (lex->current.type == TOK_RPAREN) {
                    if (depth == 0) break;
                    depth--;
                }
                lex_next(lex);
            }
        }
        if (lex->current.type == TOK_RPAREN) lex_next(lex);

        /* Save body position */
        int body_pos = lex->pos;
        int body_line = lex->line;
        Token body_tok = lex->current;

        /* Check first condition */
        if (!js_is_truthy(first_cond)) {
            JS_FreeValue(ctx, first_cond);
            /* Skip body */
            if (lex->current.type == TOK_LBRACE) {
                int depth = 1;
                lex_next(lex);
                while (!lex_eof(lex) && depth > 0) {
                    if (lex->current.type == TOK_LBRACE) depth++;
                    else if (lex->current.type == TOK_RBRACE) depth--;
                    if (depth > 0) lex_next(lex);
                }
                if (lex->current.type == TOK_RBRACE) lex_next(lex);
            }
            return JS_UNDEFINED;
        }
        JS_FreeValue(ctx, first_cond);

        /* Execute first body */
        JSValue body_result = eval_statement(ctx, lex);
        int after_body_pos = lex->pos;
        int after_body_line = lex->line;
        Token after_body_tok = lex->current;

        int is_break = JS_IS_BREAK(body_result);
        if (!is_break && !JS_IS_CONTINUE(body_result)) JS_FreeValue(ctx, body_result);
        if (is_break || ctx->has_return || has_exception(ctx)) return JS_UNDEFINED;

        /* Loop */
        for (int iter = 0; iter < 100000; iter++) {
            /* Increment */
            lex->pos = inc_pos; lex->line = inc_line; lex->current = inc_tok;
            lex->has_peek = 0;
            if (lex->current.type != TOK_RPAREN) {
                JSValue inc = eval_comma_expression(ctx, lex);
                JS_FreeValue(ctx, inc);
            }

            /* Condition */
            lex->pos = cond_pos; lex->line = cond_line; lex->current = cond_tok;
            lex->has_peek = 0;
            JSValue cond = JS_TRUE;
            if (has_cond) cond = eval_assignment(ctx, lex);
            if (!js_is_truthy(cond)) {
                JS_FreeValue(ctx, cond);
                break;
            }
            JS_FreeValue(ctx, cond);

            /* Body */
            lex->pos = body_pos; lex->line = body_line; lex->current = body_tok;
            lex->has_peek = 0;
            body_result = eval_statement(ctx, lex);
            is_break = JS_IS_BREAK(body_result);
            if (!is_break && !JS_IS_CONTINUE(body_result)) JS_FreeValue(ctx, body_result);
            if (is_break || ctx->has_return || has_exception(ctx)) break;
        }

        /* Restore position to after the for loop */
        lex->pos = after_body_pos; lex->line = after_body_line;
        lex->current = after_body_tok; lex->has_peek = 0;
        return JS_UNDEFINED;
    }

    /* Do-while */
    if (tok == TOK_DO) {
        lex_next(lex);
        int body_pos = lex->pos;
        int body_line = lex->line;
        Token body_tok = lex->current;

        for (int iter = 0; iter < 100000; iter++) {
            lex->pos = body_pos; lex->line = body_line; lex->current = body_tok;
            lex->has_peek = 0;
            JSValue body = eval_statement(ctx, lex);
            int is_break = JS_IS_BREAK(body);
            if (!is_break && !JS_IS_CONTINUE(body)) JS_FreeValue(ctx, body);
            if (is_break || ctx->has_return || has_exception(ctx)) break;

            if (lex->current.type == TOK_WHILE) lex_next(lex);
            if (lex->current.type == TOK_LPAREN) lex_next(lex);
            JSValue cond = eval_assignment(ctx, lex);
            if (lex->current.type == TOK_RPAREN) lex_next(lex);
            int cont = js_is_truthy(cond);
            JS_FreeValue(ctx, cond);
            if (!cont) break;
        }
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return JS_UNDEFINED;
    }

    /* Break / Continue */
    if (tok == TOK_BREAK) {
        lex_next(lex);
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return JS_BREAK_SIGNAL;
    }
    if (tok == TOK_CONTINUE) {
        lex_next(lex);
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        return JS_CONTINUE_SIGNAL;
    }

    /* Throw */
    if (tok == TOK_THROW) {
        lex_next(lex);
        JSValue val = eval_assignment(ctx, lex);
        if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
        ctx->current_exception = val;
        ctx->has_exception = 1;
        ctx->stats.exceptions_thrown++;
        return JS_EXCEPTION;
    }

    /* Try/catch/finally */
    if (tok == TOK_TRY) {
        lex_next(lex);
        /* Save and clear exception state before try block */
        int saved_has_exception = ctx->has_exception;
        JSValue saved_exception = ctx->current_exception;
        ctx->has_exception = 0;
        ctx->current_exception = JS_UNDEFINED;

        /* Save return state */
        int saved_has_return = ctx->has_return;
        JSValue saved_return_value = ctx->return_value;
        ctx->has_return = 0;
        ctx->return_value = JS_UNDEFINED;

        JSValue try_result = eval_block(ctx, lex);

        int had_exception = ctx->has_exception;
        JSValue exc_val = ctx->current_exception;
        int try_had_return = ctx->has_return;
        JSValue try_return_value = ctx->return_value;

        if (had_exception) {
            ctx->has_exception = 0;
            ctx->current_exception = JS_UNDEFINED;
        }

        /* Catch */
        if (lex->current.type == TOK_CATCH) {
            lex_next(lex);
            char *catch_var = NULL;
            if (lex->current.type == TOK_LPAREN) {
                lex_next(lex);
                if (lex->current.type == TOK_IDENT) {
                    catch_var = js_strndup(ctx->rt, lex->current.start, lex->current.len);
                    lex_next(lex);
                }
                if (lex->current.type == TOK_RPAREN) lex_next(lex);
            }

            if (had_exception) {
                /* Execute catch block */
                Scope *catch_scope = scope_new(ctx->rt, ctx->current_scope);
                if (catch_var) {
                    scope_define(ctx->rt, catch_scope, catch_var, exc_val);
                } else {
                    JS_FreeValue(ctx, exc_val);
                }
                Scope *saved_scope = ctx->current_scope;
                ctx->current_scope = catch_scope;
                JSValue catch_result = eval_block(ctx, lex);
                JS_FreeValue(ctx, catch_result);
                ctx->current_scope = saved_scope;
                scope_free(ctx->rt, catch_scope);
            } else {
                /* Skip catch block */
                if (lex->current.type == TOK_LBRACE) {
                    int depth = 1;
                    lex_next(lex);
                    while (!lex_eof(lex) && depth > 0) {
                        if (lex->current.type == TOK_LBRACE) depth++;
                        else if (lex->current.type == TOK_RBRACE) depth--;
                        if (depth > 0) lex_next(lex);
                    }
                    if (lex->current.type == TOK_RBRACE) lex_next(lex);
                }
            }
            if (catch_var) js_free(ctx->rt, catch_var, strlen(catch_var)+1);
        } else if (had_exception) {
            JS_FreeValue(ctx, exc_val);
        }

        /* Finally */
        if (lex->current.type == TOK_FINALLY) {
            lex_next(lex);
            JSValue finally_result = eval_block(ctx, lex);
            JS_FreeValue(ctx, finally_result);
        }

        /* Restore saved exception state if no new exception */
        if (!ctx->has_exception && saved_has_exception) {
            ctx->has_exception = saved_has_exception;
            ctx->current_exception = saved_exception;
        } else {
            JS_FreeValue(ctx, saved_exception);
        }

        /* Preserve return signal from try block */
        if (try_had_return && !had_exception) {
            ctx->has_return = 1;
            ctx->return_value = try_return_value;
            JS_FreeValue(ctx, try_result);
            /* Free old saved return state */
            if (saved_has_return)
                JS_FreeValue(ctx, saved_return_value);
            return JS_UNDEFINED;
        }
        /* Restore previous return state if try didn't return */
        if (saved_has_return && !ctx->has_return) {
            ctx->has_return = saved_has_return;
            ctx->return_value = saved_return_value;
        } else if (saved_has_return) {
            JS_FreeValue(ctx, saved_return_value);
        }
        if (try_had_return && had_exception) {
            JS_FreeValue(ctx, try_return_value);
        }

        /* Propagate break/continue from try block */
        if (JS_IS_BREAK(try_result) || JS_IS_CONTINUE(try_result)) {
            return try_result;
        }
        JS_FreeValue(ctx, try_result);
        return JS_UNDEFINED;
    }

    /* Switch */
    if (tok == TOK_SWITCH) {
        lex_next(lex);
        if (lex->current.type == TOK_LPAREN) lex_next(lex);
        JSValue disc = eval_assignment(ctx, lex);
        if (lex->current.type == TOK_RPAREN) lex_next(lex);
        if (lex->current.type == TOK_LBRACE) lex_next(lex);

        int matched = 0;
        int fell_through = 0;
        while (lex->current.type != TOK_RBRACE && lex->current.type != TOK_EOF) {
            if (lex->current.type == TOK_CASE) {
                lex_next(lex);
                JSValue case_val = eval_assignment(ctx, lex);
                if (lex->current.type == TOK_COLON) lex_next(lex);

                if (!matched && !fell_through) {
                    matched = js_strict_eq(disc, case_val);
                }
                JS_FreeValue(ctx, case_val);

                if (matched || fell_through) {
                    fell_through = 1;
                    while (lex->current.type != TOK_CASE &&
                           lex->current.type != TOK_DEFAULT &&
                           lex->current.type != TOK_RBRACE &&
                           lex->current.type != TOK_EOF) {
                        JSValue stmt = eval_statement(ctx, lex);
                        if (JS_IS_BREAK(stmt)) {
                            fell_through = 0;
                            matched = 0;
                            goto switch_done;
                        }
                        JS_FreeValue(ctx, stmt);
                    }
                } else {
                    /* Skip case body */
                    int depth = 0;
                    while (lex->current.type != TOK_EOF) {
                        if (lex->current.type == TOK_LBRACE) depth++;
                        else if (lex->current.type == TOK_RBRACE) {
                            if (depth == 0) break;
                            depth--;
                        }
                        if ((lex->current.type == TOK_CASE || lex->current.type == TOK_DEFAULT) && depth == 0)
                            break;
                        lex_next(lex);
                    }
                }
            } else if (lex->current.type == TOK_DEFAULT) {
                lex_next(lex);
                if (lex->current.type == TOK_COLON) lex_next(lex);
                if (!matched) fell_through = 1;
                if (fell_through) {
                    while (lex->current.type != TOK_CASE &&
                           lex->current.type != TOK_RBRACE &&
                           lex->current.type != TOK_EOF) {
                        JSValue stmt = eval_statement(ctx, lex);
                        if (JS_IS_BREAK(stmt)) { fell_through = 0; goto switch_done; }
                        JS_FreeValue(ctx, stmt);
                    }
                }
            } else {
                lex_next(lex); /* skip unexpected token */
            }
        }
switch_done:
        while (lex->current.type != TOK_RBRACE && lex->current.type != TOK_EOF)
            lex_next(lex);
        if (lex->current.type == TOK_RBRACE) lex_next(lex);
        JS_FreeValue(ctx, disc);
        return JS_UNDEFINED;
    }

    /* Class - skip for now */
    if (tok == TOK_CLASS) {
        /* Skip entire class declaration */
        lex_next(lex);
        if (lex->current.type == TOK_IDENT) lex_next(lex);
        if (lex->current.type == TOK_EXTENDS) {
            lex_next(lex); lex_next(lex);
        }
        if (lex->current.type == TOK_LBRACE) {
            int depth = 1;
            lex_next(lex);
            while (!lex_eof(lex) && depth > 0) {
                if (lex->current.type == TOK_LBRACE) depth++;
                else if (lex->current.type == TOK_RBRACE) depth--;
                if (depth > 0) lex_next(lex);
            }
            if (lex->current.type == TOK_RBRACE) lex_next(lex);
        }
        return JS_UNDEFINED;
    }

    /* Expression statement (comma operator allowed here) */
    JSValue result = eval_comma_expression(ctx, lex);
    if (lex->current.type == TOK_SEMICOLON) lex_next(lex);
    return result;
}

/* Block: { statements } */
static JSValue eval_block(JSContext *ctx, Lexer *lex) {
    if (lex->current.type != TOK_LBRACE)
        return eval_statement(ctx, lex);

    lex_next(lex); /* skip { */
    Scope *block_scope = scope_new(ctx->rt, ctx->current_scope);
    Scope *saved = ctx->current_scope;
    ctx->current_scope = block_scope;

    JSValue last = JS_UNDEFINED;
    while (lex->current.type != TOK_RBRACE && lex->current.type != TOK_EOF) {
        JS_FreeValue(ctx, last);
        last = eval_statement(ctx, lex);
        if (has_exception(ctx) || ctx->has_return ||
            JS_IS_BREAK(last) || JS_IS_CONTINUE(last)) break;
    }
    if (lex->current.type == TOK_RBRACE) lex_next(lex);

    ctx->current_scope = saved;
    scope_free(ctx->rt, block_scope);
    return last;
}

/* Parse a sequence of statements */
static JSValue eval_statement_list(JSContext *ctx, Lexer *lex) {
    JSValue last = JS_UNDEFINED;
    while (lex->current.type != TOK_EOF && lex->current.type != TOK_RBRACE) {
        JS_FreeValue(ctx, last);
        last = eval_statement(ctx, lex);
        if (has_exception(ctx) || ctx->has_return ||
            JS_IS_BREAK(last) || JS_IS_CONTINUE(last)) break;
    }
    return last;
}

/* Main eval entry point */
static JSValue js_eval_source(JSContext *ctx, const char *input, size_t len) {
    Lexer lex;
    lexer_init(&lex, input, (int)len);
    lex_next(&lex); /* prime the lexer */
    return eval_statement_list(ctx, &lex);
}


/* ================================================================
 * Section 11: Public API Implementation
 * ================================================================ */

/* ── Runtime ── */

JSRuntime *JS_NewRuntime(void) {
    JSRuntime *rt = (JSRuntime *)calloc(1, sizeof(JSRuntime));
    if (!rt) return NULL;
    rt->max_stack_size = 256 * 1024;
    return rt;
}

void JS_FreeRuntime(JSRuntime *rt) {
    if (rt) free(rt);
}

void JS_SetMemoryLimit(JSRuntime *rt, size_t limit) {
    if (rt) rt->memory_limit = limit;
}

void JS_SetMaxStackSize(JSRuntime *rt, size_t stack_size) {
    if (rt) rt->max_stack_size = stack_size;
}

void JS_SetInterruptHandler(JSRuntime *rt, JSInterruptHandler handler,
                            void *opaque) {
    if (rt) {
        rt->interrupt_handler = handler;
        rt->interrupt_opaque = opaque;
    }
}

/* ── Context ── */

JSContext *JS_NewContext(JSRuntime *rt) {
    JSContext *ctx = (JSContext *)calloc(1, sizeof(JSContext));
    if (!ctx) return NULL;
    ctx->rt = rt;
    ctx->has_exception = 0;
    ctx->current_exception = JS_UNDEFINED;
    ctx->opaque = NULL;
    ctx->scope_depth = 0;
    ctx->stmt_counter = 0;
    memset(&ctx->stats, 0, sizeof(JSExecStats));

    /* Create global scope */
    ctx->current_scope = scope_new(rt, NULL);

    /* Create global object */
    ctx->global_obj = js_mkobject(ctx);

    return ctx;
}

void JS_FreeContext(JSContext *ctx) {
    if (!ctx) return;
    JS_FreeValue(ctx, ctx->global_obj);
    if (ctx->has_exception)
        js_free_value_rt(ctx->rt, ctx->current_exception);
    if (ctx->current_scope)
        scope_free(ctx->rt, ctx->current_scope);
    if (ctx->output_buf)
        free(ctx->output_buf);
    free(ctx);
}

JSRuntime *JS_GetRuntime(JSContext *ctx) {
    return ctx ? ctx->rt : NULL;
}

/* ── Eval ── */

JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len,
                const char *filename, int eval_flags) {
    if (!ctx || !input) return JS_EXCEPTION;

    /* Reset stats */
    memset(&ctx->stats, 0, sizeof(JSExecStats));
    ctx->has_return = 0;
    ctx->return_value = JS_UNDEFINED;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Clear any previous exception */
    if (ctx->has_exception) {
        js_free_value_rt(ctx->rt, ctx->current_exception);
        ctx->has_exception = 0;
        ctx->current_exception = JS_UNDEFINED;
    }

    JSValue result = js_eval_source(ctx, input, input_len);

    /* If function returned, use the return value */
    if (ctx->has_return) {
        JS_FreeValue(ctx, result);
        result = ctx->return_value; /* take ownership */
        ctx->return_value = JS_UNDEFINED;
        ctx->has_return = 0;
    }

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    ctx->stats.elapsed_ms =
        (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
        (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    return result;
}

/* ── Value creation ── */

JSValue JS_NewString(JSContext *ctx, const char *str) {
    if (!ctx || !str) return JS_EXCEPTION;
    return js_mkstring(ctx, str, strlen(str));
}

JSValue JS_NewStringLen(JSContext *ctx, const char *str, size_t len) {
    if (!ctx || !str) return JS_EXCEPTION;
    return js_mkstring(ctx, str, len);
}

JSValue JS_NewInt32(JSContext *ctx, int32_t val) {
    (void)ctx;
    return JS_MKVAL(JS_TAG_INT, (uint64_t)(uint32_t)val);
}

JSValue JS_NewFloat64(JSContext *ctx, double val) {
    return js_mkfloat(ctx, val);
}

JSValue JS_NewBool(JSContext *ctx, int val) {
    (void)ctx;
    return val ? JS_TRUE : JS_FALSE;
}

JSValue JS_NewObject(JSContext *ctx) {
    return js_mkobject(ctx);
}

JSValue JS_NewArray(JSContext *ctx) {
    return js_mkarray(ctx);
}

/* ── Value extraction ── */

const char *JS_ToCString(JSContext *ctx, JSValueConst val) {
    JSValue str = js_tostring(ctx, val);
    if (JS_IsException(str)) return NULL;
    JSString_s *s = js_get_string(str);
    if (!s) return NULL;
    /* Return a copy that the caller must free with JS_FreeCString */
    char *copy = js_strdup(ctx->rt, s->data);
    JS_FreeValue(ctx, str);
    return copy;
}

void JS_FreeCString(JSContext *ctx, const char *ptr) {
    if (ptr) js_free(ctx->rt, (void *)ptr, strlen(ptr) + 1);
}

int JS_ToInt32(JSContext *ctx, int32_t *pres, JSValueConst val) {
    (void)ctx;
    double d = js_value_to_number(val);
    if (pres) *pres = (int32_t)d;
    return isnan(d) ? -1 : 0;
}

int JS_ToFloat64(JSContext *ctx, double *pres, JSValueConst val) {
    (void)ctx;
    double d = js_value_to_number(val);
    if (pres) *pres = d;
    return 0;
}

int JS_ToBool(JSContext *ctx, JSValueConst val) {
    (void)ctx;
    return js_is_truthy(val);
}

/* ── Reference counting ── */

JSValue JS_DupValue(JSContext *ctx, JSValueConst val) {
    (void)ctx;
    return js_dup_value(val);
}

void JS_FreeValue(JSContext *ctx, JSValue val) {
    js_free_value_rt(ctx->rt, val);
}

/* ── Object property access ── */

JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                          const char *prop) {
    JSObject_s *o = js_get_object(this_obj);
    if (!o) return JS_UNDEFINED;
    return obj_get_prop(o, prop);
}

int JS_SetPropertyStr(JSContext *ctx, JSValue this_obj,
                      const char *prop, JSValue val) {
    JSObject_s *o = js_get_object(this_obj);
    if (!o) { JS_FreeValue(ctx, val); return -1; }
    return obj_set_prop(ctx->rt, o, prop, val);
}

int JS_DeletePropertyStr(JSContext *ctx, JSValue obj, const char *prop) {
    JSObject_s *o = js_get_object(obj);
    if (!o) return -1;
    return obj_delete_prop(ctx->rt, o, prop);
}

int JS_HasPropertyStr(JSContext *ctx, JSValueConst obj, const char *prop) {
    JSObject_s *o = js_get_object(obj);
    if (!o) return 0;
    return obj_has_prop(o, prop);
}

/* ── Array operations ── */

int JS_SetPropertyUint32(JSContext *ctx, JSValue obj, uint32_t idx,
                         JSValue val) {
    JSObject_s *o = js_get_object(obj);
    if (!o) { JS_FreeValue(ctx, val); return -1; }
    if (!o->array_data) {
        o->array_cap = (int)(idx + 1) > 8 ? (int)(idx + 1) * 2 : 8;
        o->array_data = (JSValue *)js_malloc(ctx->rt,
            sizeof(JSValue) * o->array_cap);
        if (!o->array_data) { JS_FreeValue(ctx, val); return -1; }
        for (int i = 0; i < o->array_cap; i++)
            o->array_data[i] = JS_UNDEFINED;
        o->array_len = 0;
    }
    if ((int)idx >= o->array_cap) {
        int new_cap = (int)(idx + 1) * 2;
        JSValue *new_data = (JSValue *)js_realloc(ctx->rt, o->array_data,
            sizeof(JSValue) * o->array_cap, sizeof(JSValue) * new_cap);
        if (!new_data) { JS_FreeValue(ctx, val); return -1; }
        for (int i = o->array_cap; i < new_cap; i++)
            new_data[i] = JS_UNDEFINED;
        o->array_data = new_data;
        o->array_cap = new_cap;
    }
    if ((int)idx < o->array_len)
        js_free_value_rt(ctx->rt, o->array_data[idx]);
    o->array_data[idx] = val;
    if ((int)idx >= o->array_len)
        o->array_len = (int)idx + 1;
    return 0;
}

JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx) {
    JSObject_s *o = js_get_object(obj);
    if (!o || !o->array_data || (int)idx >= o->array_len)
        return JS_UNDEFINED;
    return js_dup_value(o->array_data[idx]);
}

/* ── Function creation ── */

JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name,
                        int length) {
    return js_mkfunc_c(ctx, func, name, length);
}

/* ── Global object ── */

JSValue JS_GetGlobalObject(JSContext *ctx) {
    if (!ctx) return JS_EXCEPTION;
    return js_dup_value(ctx->global_obj);
}

/* ── Exception handling ── */

JSValue JS_GetException(JSContext *ctx) {
    if (!ctx || !ctx->has_exception) return JS_NULL;
    JSValue exc = ctx->current_exception;
    ctx->current_exception = JS_UNDEFINED;
    ctx->has_exception = 0;
    return exc;
}

JSValue JS_Throw(JSContext *ctx, JSValue obj) {
    if (ctx->has_exception)
        js_free_value_rt(ctx->rt, ctx->current_exception);
    ctx->current_exception = obj;
    ctx->has_exception = 1;
    ctx->stats.exceptions_thrown++;
    return JS_EXCEPTION;
}

JSValue JS_ThrowTypeError(JSContext *ctx, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return JS_Throw(ctx, JS_NewString(ctx, buf));
}

JSValue JS_ThrowReferenceError(JSContext *ctx, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return JS_Throw(ctx, JS_NewString(ctx, buf));
}

/* ── Pane-specific extensions ── */

void JS_SetContextOpaque(JSContext *ctx, void *opaque) {
    if (ctx) ctx->opaque = opaque;
}

void *JS_GetContextOpaque(JSContext *ctx) {
    return ctx ? ctx->opaque : NULL;
}

int JS_GetExecStats(JSContext *ctx, JSExecStats *stats) {
    if (!ctx || !stats) return -1;
    *stats = ctx->stats;
    return 0;
}


/* ================================================================
 * Section 12: Output Buffer and Browser Globals
 * ================================================================ */

/* Output buffer management */
static void output_buf_init(JSContext *ctx) {
    ctx->output_cap = 4096;
    ctx->output_buf = (char *)malloc(ctx->output_cap);
    if (!ctx->output_buf) { ctx->output_cap = 0; return; }
    ctx->output_buf[0] = '\0';
    ctx->output_len = 0;
}

static void output_buf_append(JSContext *ctx, const char *type, const char *data) {
    if (!ctx->output_buf) return;
    size_t type_len = strlen(type);
    size_t data_len = data ? strlen(data) : 0;
    /* Sanitize embedded newlines in data to prevent output buffer protocol corruption */
    char *sanitized = NULL;
    if (data && data_len > 0) {
        for (size_t i = 0; i < data_len; i++) {
            if (data[i] == '\n') {
                sanitized = (char *)malloc(data_len + 1);
                if (!sanitized) return;
                memcpy(sanitized, data, data_len);
                for (size_t j = i; j < data_len; j++) {
                    if (sanitized[j] == '\n') sanitized[j] = ' ';
                }
                sanitized[data_len] = '\0';
                data = sanitized;
                break;
            }
        }
    }
    size_t needed = ctx->output_len + type_len + 1 + data_len + 1; /* type:data\n */
    if (needed >= ctx->output_cap) {
        size_t new_cap = ctx->output_cap * 2;
        while (new_cap < needed + 1) new_cap *= 2;
        char *new_buf = (char *)realloc(ctx->output_buf, new_cap);
        if (!new_buf) { free(sanitized); return; }
        ctx->output_buf = new_buf;
        ctx->output_cap = new_cap;
    }
    memcpy(ctx->output_buf + ctx->output_len, type, type_len);
    ctx->output_len += type_len;
    ctx->output_buf[ctx->output_len++] = ':';
    if (data && data_len > 0) {
        memcpy(ctx->output_buf + ctx->output_len, data, data_len);
        ctx->output_len += data_len;
    }
    ctx->output_buf[ctx->output_len++] = '\n';
    ctx->output_buf[ctx->output_len] = '\0';
    free(sanitized);
}

const char *JS_GetOutputBuffer(JSContext *ctx) {
    if (!ctx || !ctx->output_buf) return "";
    return ctx->output_buf;
}

void JS_ClearOutputBuffer(JSContext *ctx) {
    if (!ctx || !ctx->output_buf) return;
    ctx->output_buf[0] = '\0';
    ctx->output_len = 0;
}

/* Helper: extract string argument from JSValue.
 * Returns a heap-allocated string (free with JS_FreeCString) or NULL. */
static const char *extract_arg_string(JSContext *ctx, int argc, JSValueConst *argv, int idx) {
    if (idx >= argc) return NULL;
    return JS_ToCString(ctx, argv[idx]);
}

/* ── Built-in browser functions ── */

/* console.log / warn / error / info */
static JSValue js_console_log(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val;
    /* Concatenate all arguments separated by spaces */
    char buf[2048];
    int pos = 0;
    for (int i = 0; i < argc && pos < 2040; i++) {
        if (i > 0) buf[pos++] = ' ';
        const char *s = JS_ToCString(ctx, argv[i]);
        if (s) {
            int slen = strlen(s);
            if (pos + slen > 2040) slen = 2040 - pos;
            memcpy(buf + pos, s, slen);
            pos += slen;
            JS_FreeCString(ctx, s);
        }
    }
    buf[pos] = '\0';
    output_buf_append(ctx, "LOG", buf);
    return JS_UNDEFINED;
}

static JSValue js_console_warn(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    (void)this_val;
    char buf[2048];
    int pos = 0;
    for (int i = 0; i < argc && pos < 2040; i++) {
        if (i > 0) buf[pos++] = ' ';
        const char *s = JS_ToCString(ctx, argv[i]);
        if (s) {
            int slen = strlen(s);
            if (pos + slen > 2040) slen = 2040 - pos;
            memcpy(buf + pos, s, slen);
            pos += slen;
            JS_FreeCString(ctx, s);
        }
    }
    buf[pos] = '\0';
    output_buf_append(ctx, "WARN", buf);
    return JS_UNDEFINED;
}

static JSValue js_console_error(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    (void)this_val;
    char buf[2048];
    int pos = 0;
    for (int i = 0; i < argc && pos < 2040; i++) {
        if (i > 0) buf[pos++] = ' ';
        const char *s = JS_ToCString(ctx, argv[i]);
        if (s) {
            int slen = strlen(s);
            if (pos + slen > 2040) slen = 2040 - pos;
            memcpy(buf + pos, s, slen);
            pos += slen;
            JS_FreeCString(ctx, s);
        }
    }
    buf[pos] = '\0';
    output_buf_append(ctx, "ERROR", buf);
    return JS_UNDEFINED;
}

/* alert() */
static JSValue js_alert(JSContext *ctx, JSValueConst this_val,
                         int argc, JSValueConst *argv) {
    (void)this_val;
    const char *msg = extract_arg_string(ctx, argc, argv, 0);
    output_buf_append(ctx, "ALERT", msg ? msg : "");
    if (msg) JS_FreeCString(ctx, msg);
    return JS_UNDEFINED;
}

/* document.write / document.writeln */
static JSValue js_document_write(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)this_val;
    char buf[4096];
    int pos = 0;
    for (int i = 0; i < argc && pos < 4080; i++) {
        const char *s = JS_ToCString(ctx, argv[i]);
        if (s) {
            int slen = strlen(s);
            if (pos + slen > 4080) slen = 4080 - pos;
            memcpy(buf + pos, s, slen);
            pos += slen;
            JS_FreeCString(ctx, s);
        }
    }
    buf[pos] = '\0';
    output_buf_append(ctx, "DOM_WRITE", buf);
    return JS_UNDEFINED;
}

/* document.getElementById */
static JSValue js_get_element_by_id(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)this_val;
    const char *id = extract_arg_string(ctx, argc, argv, 0);
    if (!id) return JS_NULL;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "getElementById:%s", id);
    output_buf_append(ctx, "DOM_QUERY", cmd);
    /* Create placeholder object BEFORE freeing the id string */
    JSValue obj = JS_NewObject(ctx);
    JSValue id_val = JS_NewString(ctx, id);
    JS_FreeCString(ctx, id);
    JS_SetPropertyStr(ctx, obj, "__element_id__", id_val);
    return obj;
}

/* document.querySelector */
static JSValue js_query_selector(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)this_val;
    const char *sel = extract_arg_string(ctx, argc, argv, 0);
    if (!sel) return JS_NULL;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "querySelector:%s", sel);
    output_buf_append(ctx, "DOM_QUERY", cmd);
    JS_FreeCString(ctx, sel);
    return JS_NewObject(ctx);
}

/* document.createElement */
static JSValue js_create_element(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)this_val;
    const char *tag = extract_arg_string(ctx, argc, argv, 0);
    if (!tag) return JS_NULL;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "createElement:%s", tag);
    output_buf_append(ctx, "DOM_CREATE", cmd);
    JS_FreeCString(ctx, tag);
    return JS_NewObject(ctx);
}

/* window.location setter handler */
static JSValue js_set_location(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    (void)this_val;
    const char *url = extract_arg_string(ctx, argc, argv, 0);
    if (url) {
        output_buf_append(ctx, "NAV", url);
        JS_FreeCString(ctx, url);
    }
    return JS_UNDEFINED;
}

/* document.cookie getter/setter */
static JSValue js_document_cookie(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc > 0) {
        const char *val = extract_arg_string(ctx, argc, argv, 0);
        if (val) {
            output_buf_append(ctx, "COOKIE", val);
            JS_FreeCString(ctx, val);
        }
    }
    return JS_NewString(ctx, "");
}

/* setTimeout/setInterval - log and return 0 */
static JSValue js_set_timeout(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val;
    output_buf_append(ctx, "TIMER", "setTimeout");
    /* If first arg is a function, try to call it */
    if (argc > 0 && JS_IsFunction(argv[0])) {
        JSValue result = JS_UNDEFINED; /* Don't execute timers for safety */
        (void)result;
    }
    return JS_NewInt32(ctx, 0);
}

static JSValue js_set_interval(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    output_buf_append(ctx, "TIMER", "setInterval");
    return JS_NewInt32(ctx, 0);
}

static JSValue js_clear_timer(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
}

/* fetch / XMLHttpRequest stub */
static JSValue js_fetch_stub(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    (void)this_val;
    const char *url = extract_arg_string(ctx, argc, argv, 0);
    if (url) {
        output_buf_append(ctx, "FETCH", url);
        JS_FreeCString(ctx, url);
    }
    /* Return a promise-like object (with then/catch as no-ops) */
    JSValue obj = JS_NewObject(ctx);
    return obj;
}

/* addEventListener (no-op, just log) */
static JSValue js_add_event_listener(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)this_val;
    const char *event = extract_arg_string(ctx, argc, argv, 0);
    if (event) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "addEventListener:%s", event);
        output_buf_append(ctx, "EVENT", cmd);
        JS_FreeCString(ctx, event);
    }
    return JS_UNDEFINED;
}

/* JSON.stringify */
static JSValue js_json_stringify(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_NewString(ctx, "undefined");
    return js_tostring(ctx, argv[0]);
}

/* JSON.parse (simplified: just return the string for now) */
static JSValue js_json_parse(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_UNDEFINED;
    /* For a real implementation we'd parse JSON into objects.
       For now, return the value as-is if it's a string. */
    if (JS_IsString(argv[0])) {
        return JS_DupValue(ctx, argv[0]);
    }
    return JS_UNDEFINED;
}

/* parseInt */
static JSValue js_parse_int(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_NewFloat64(ctx, NAN);
    const char *s = JS_ToCString(ctx, argv[0]);
    if (!s) return JS_NewFloat64(ctx, NAN);
    int base = 10;
    if (argc > 1) {
        int32_t b;
        JS_ToInt32(ctx, &b, argv[1]);
        if (b >= 2 && b <= 36) base = b;
    }
    long val = strtol(s, NULL, base);
    JS_FreeCString(ctx, s);
    return JS_NewInt32(ctx, (int32_t)val);
}

/* parseFloat */
static JSValue js_parse_float(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_NewFloat64(ctx, NAN);
    const char *s = JS_ToCString(ctx, argv[0]);
    if (!s) return JS_NewFloat64(ctx, NAN);
    double val = strtod(s, NULL);
    JS_FreeCString(ctx, s);
    return JS_NewFloat64(ctx, val);
}

/* isNaN */
static JSValue js_is_nan(JSContext *ctx, JSValueConst this_val,
                          int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_TRUE;
    double d;
    JS_ToFloat64(ctx, &d, argv[0]);
    return isnan(d) ? JS_TRUE : JS_FALSE;
}

/* encodeURIComponent (simplified) */
static JSValue js_encode_uri_component(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_NewString(ctx, "undefined");
    return JS_DupValue(ctx, argv[0]); /* simplified: return as-is */
}

/* ── Install browser globals ── */

static void install_method(JSContext *ctx, JSValue obj, const char *name,
                           JSCFunction func, int argc) {
    JSValue fn = JS_NewCFunction(ctx, &func, name, argc);
    JS_SetPropertyStr(ctx, obj, name, fn);
}

void JS_InstallBrowserGlobals(JSContext *ctx) {
    if (!ctx) return;

    /* Initialize output buffer */
    if (!ctx->output_buf) output_buf_init(ctx);

    JSValue global = JS_GetGlobalObject(ctx);

    /* console object */
    JSValue console_obj = JS_NewObject(ctx);
    install_method(ctx, console_obj, "log", js_console_log, 1);
    install_method(ctx, console_obj, "warn", js_console_warn, 1);
    install_method(ctx, console_obj, "error", js_console_error, 1);
    install_method(ctx, console_obj, "info", js_console_log, 1);
    install_method(ctx, console_obj, "debug", js_console_log, 1);
    install_method(ctx, console_obj, "dir", js_console_log, 1);
    JS_SetPropertyStr(ctx, global, "console", console_obj);

    /* alert */
    install_method(ctx, global, "alert", js_alert, 1);

    /* document object */
    JSValue doc = JS_NewObject(ctx);
    install_method(ctx, doc, "getElementById", js_get_element_by_id, 1);
    install_method(ctx, doc, "querySelector", js_query_selector, 1);
    install_method(ctx, doc, "querySelectorAll", js_query_selector, 1);
    install_method(ctx, doc, "createElement", js_create_element, 1);
    install_method(ctx, doc, "write", js_document_write, 1);
    install_method(ctx, doc, "writeln", js_document_write, 1);
    install_method(ctx, doc, "addEventListener", js_add_event_listener, 2);
    /* document.cookie as a function (simplified) */
    install_method(ctx, doc, "getCookie", js_document_cookie, 0);
    install_method(ctx, doc, "setCookie", js_document_cookie, 1);
    /* document.title */
    JS_SetPropertyStr(ctx, doc, "title", JS_NewString(ctx, ""));
    /* document.readyState */
    JS_SetPropertyStr(ctx, doc, "readyState", JS_NewString(ctx, "complete"));
    /* document.documentElement placeholder */
    JSValue html_elem = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, html_elem, "tagName", JS_NewString(ctx, "HTML"));
    JS_SetPropertyStr(ctx, doc, "documentElement", html_elem);
    /* document.body placeholder */
    JSValue body_elem = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, body_elem, "tagName", JS_NewString(ctx, "BODY"));
    install_method(ctx, body_elem, "appendChild", js_create_element, 1);
    install_method(ctx, body_elem, "addEventListener", js_add_event_listener, 2);
    JS_SetPropertyStr(ctx, doc, "body", body_elem);
    /* document.head placeholder */
    JSValue head_elem = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, head_elem, "tagName", JS_NewString(ctx, "HEAD"));
    install_method(ctx, head_elem, "appendChild", js_create_element, 1);
    JS_SetPropertyStr(ctx, doc, "head", head_elem);
    JS_SetPropertyStr(ctx, global, "document", doc);

    /* window object (alias for global + extra props) */
    JSValue window = JS_DupValue(ctx, global);
    install_method(ctx, window, "addEventListener", js_add_event_listener, 2);
    install_method(ctx, window, "setTimeout", js_set_timeout, 2);
    install_method(ctx, window, "setInterval", js_set_interval, 2);
    install_method(ctx, window, "clearTimeout", js_clear_timer, 1);
    install_method(ctx, window, "clearInterval", js_clear_timer, 1);
    install_method(ctx, window, "fetch", js_fetch_stub, 1);
    /* window.location */
    JSValue location = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, location, "href", JS_NewString(ctx, ""));
    JS_SetPropertyStr(ctx, location, "hostname", JS_NewString(ctx, ""));
    JS_SetPropertyStr(ctx, location, "pathname", JS_NewString(ctx, "/"));
    JS_SetPropertyStr(ctx, location, "protocol", JS_NewString(ctx, "https:"));
    JS_SetPropertyStr(ctx, location, "search", JS_NewString(ctx, ""));
    JS_SetPropertyStr(ctx, location, "hash", JS_NewString(ctx, ""));
    install_method(ctx, location, "assign", js_set_location, 1);
    install_method(ctx, location, "replace", js_set_location, 1);
    JS_SetPropertyStr(ctx, window, "location", location);
    /* window.navigator */
    JSValue navigator = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, navigator, "userAgent",
                      JS_NewString(ctx, "Pane/1.0"));
    JS_SetPropertyStr(ctx, navigator, "language",
                      JS_NewString(ctx, "en-US"));
    install_method(ctx, navigator, "sendBeacon", js_fetch_stub, 2);
    JS_SetPropertyStr(ctx, window, "navigator", navigator);
    JS_SetPropertyStr(ctx, global, "window", window);
    JS_SetPropertyStr(ctx, global, "self", JS_DupValue(ctx, window));

    /* Global functions */
    install_method(ctx, global, "setTimeout", js_set_timeout, 2);
    install_method(ctx, global, "setInterval", js_set_interval, 2);
    install_method(ctx, global, "clearTimeout", js_clear_timer, 1);
    install_method(ctx, global, "clearInterval", js_clear_timer, 1);
    install_method(ctx, global, "parseInt", js_parse_int, 2);
    install_method(ctx, global, "parseFloat", js_parse_float, 1);
    install_method(ctx, global, "isNaN", js_is_nan, 1);
    install_method(ctx, global, "encodeURIComponent", js_encode_uri_component, 1);
    install_method(ctx, global, "decodeURIComponent", js_encode_uri_component, 1);
    install_method(ctx, global, "encodeURI", js_encode_uri_component, 1);
    install_method(ctx, global, "decodeURI", js_encode_uri_component, 1);
    install_method(ctx, global, "fetch", js_fetch_stub, 1);

    /* JSON object */
    JSValue json = JS_NewObject(ctx);
    install_method(ctx, json, "stringify", js_json_stringify, 1);
    install_method(ctx, json, "parse", js_json_parse, 1);
    JS_SetPropertyStr(ctx, global, "JSON", json);

    /* Math object */
    JSValue math = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, math, "PI", JS_NewFloat64(ctx, 3.14159265358979323846));
    JS_SetPropertyStr(ctx, math, "E", JS_NewFloat64(ctx, 2.71828182845904523536));
    JS_SetPropertyStr(ctx, global, "Math", math);

    /* Boolean, Number, String constructors (simplified - just return the value) */
    install_method(ctx, global, "Boolean", js_parse_int, 1);
    install_method(ctx, global, "Number", js_parse_float, 1);
    install_method(ctx, global, "String", js_json_stringify, 1);

    /* Object.keys etc. */
    JSValue object_ctor = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, global, "Object", object_ctor);

    /* Array constructor */
    JSValue array_ctor = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, global, "Array", array_ctor);

    JS_FreeValue(ctx, global);
}

