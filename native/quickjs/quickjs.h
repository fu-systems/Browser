/*
 * quickjs.h — QuickJS-compatible public API header for Pane browser.
 *
 * Provides the subset of the QuickJS C API needed by the Pane browser's
 * JavaScript engine. This header defines the types and function signatures
 * that the Dart FFI layer binds against.
 */

#ifndef QUICKJS_H
#define QUICKJS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Opaque types ─────────────────────────────────────────────── */

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

/* ── JSValue ──────────────────────────────────────────────────── */

/*
 * JSValue is a tagged union. We use NaN-boxing on 64-bit:
 * - Lower 48 bits: payload (pointer or integer)
 * - Upper 16 bits: tag
 */
typedef uint64_t JSValue;
typedef JSValue JSValueConst;

/* Tags stored in upper 16 bits */
#define JS_TAG_UNDEFINED  0
#define JS_TAG_NULL       1
#define JS_TAG_BOOL       2
#define JS_TAG_INT        3
#define JS_TAG_FLOAT64    4
#define JS_TAG_STRING     5
#define JS_TAG_OBJECT     6
#define JS_TAG_FUNCTION   7
#define JS_TAG_EXCEPTION  8
#define JS_TAG_ARRAY      9
#define JS_TAG_C_FUNCTION 10

/* Construct / extract */
#define JS_MKVAL(tag, payload) \
    (((uint64_t)(tag) << 48) | ((uint64_t)(payload) & 0xFFFFFFFFFFFFULL))
#define JS_VALUE_GET_TAG(v)     ((int)((v) >> 48))
#define JS_VALUE_GET_PTR(v)     ((void*)(uintptr_t)((v) & 0xFFFFFFFFFFFFULL))
#define JS_VALUE_GET_INT(v)     ((int32_t)((v) & 0xFFFFFFFFULL))
#define JS_VALUE_GET_BOOL(v)    ((int)((v) & 1))

/* Predefined constants */
#define JS_UNDEFINED    JS_MKVAL(JS_TAG_UNDEFINED, 0)
#define JS_NULL         JS_MKVAL(JS_TAG_NULL, 0)
#define JS_TRUE         JS_MKVAL(JS_TAG_BOOL, 1)
#define JS_FALSE        JS_MKVAL(JS_TAG_BOOL, 0)
#define JS_EXCEPTION    JS_MKVAL(JS_TAG_EXCEPTION, 0)

/* Type checking */
#define JS_IsUndefined(v)   (JS_VALUE_GET_TAG(v) == JS_TAG_UNDEFINED)
#define JS_IsNull(v)        (JS_VALUE_GET_TAG(v) == JS_TAG_NULL)
#define JS_IsBool(v)        (JS_VALUE_GET_TAG(v) == JS_TAG_BOOL)
#define JS_IsNumber(v)      (JS_VALUE_GET_TAG(v) == JS_TAG_INT || \
                             JS_VALUE_GET_TAG(v) == JS_TAG_FLOAT64)
#define JS_IsString(v)      (JS_VALUE_GET_TAG(v) == JS_TAG_STRING)
#define JS_IsObject(v)      (JS_VALUE_GET_TAG(v) == JS_TAG_OBJECT)
#define JS_IsFunction(v)    (JS_VALUE_GET_TAG(v) == JS_TAG_FUNCTION || \
                             JS_VALUE_GET_TAG(v) == JS_TAG_C_FUNCTION)
#define JS_IsArray(v)       (JS_VALUE_GET_TAG(v) == JS_TAG_ARRAY)
#define JS_IsException(v)   (JS_VALUE_GET_TAG(v) == JS_TAG_EXCEPTION)

/* ── C function callback type ─────────────────────────────────── */

typedef JSValue (*JSCFunction)(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);

/* ── Eval flags ───────────────────────────────────────────────── */

#define JS_EVAL_TYPE_GLOBAL   0
#define JS_EVAL_TYPE_MODULE   1
#define JS_EVAL_FLAG_STRICT   (1 << 3)

/* ── Runtime API ──────────────────────────────────────────────── */

JSRuntime *JS_NewRuntime(void);
void       JS_FreeRuntime(JSRuntime *rt);
void       JS_SetMemoryLimit(JSRuntime *rt, size_t limit);
void       JS_SetMaxStackSize(JSRuntime *rt, size_t stack_size);

/* Interrupt handler: return non-zero to abort execution */
typedef int (*JSInterruptHandler)(JSRuntime *rt, void *opaque);
void JS_SetInterruptHandler(JSRuntime *rt, JSInterruptHandler handler,
                            void *opaque);

/* ── Context API ──────────────────────────────────────────────── */

JSContext *JS_NewContext(JSRuntime *rt);
void       JS_FreeContext(JSContext *ctx);
JSRuntime *JS_GetRuntime(JSContext *ctx);

/* ── Eval ─────────────────────────────────────────────────────── */

JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len,
                const char *filename, int eval_flags);

/* ── Value creation ───────────────────────────────────────────── */

JSValue JS_NewString(JSContext *ctx, const char *str);
JSValue JS_NewStringLen(JSContext *ctx, const char *str, size_t len);
JSValue JS_NewInt32(JSContext *ctx, int32_t val);
JSValue JS_NewFloat64(JSContext *ctx, double val);
JSValue JS_NewBool(JSContext *ctx, int val);
JSValue JS_NewObject(JSContext *ctx);
JSValue JS_NewArray(JSContext *ctx);

/* ── Value extraction ─────────────────────────────────────────── */

const char *JS_ToCString(JSContext *ctx, JSValueConst val);
void        JS_FreeCString(JSContext *ctx, const char *ptr);
int         JS_ToInt32(JSContext *ctx, int32_t *pres, JSValueConst val);
int         JS_ToFloat64(JSContext *ctx, double *pres, JSValueConst val);
int         JS_ToBool(JSContext *ctx, JSValueConst val);

/* ── Reference counting ───────────────────────────────────────── */

JSValue JS_DupValue(JSContext *ctx, JSValueConst val);
void    JS_FreeValue(JSContext *ctx, JSValue val);

/* ── Object property access ───────────────────────────────────── */

JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                          const char *prop);
int     JS_SetPropertyStr(JSContext *ctx, JSValue this_obj,
                          const char *prop, JSValue val);
int     JS_DeletePropertyStr(JSContext *ctx, JSValue obj, const char *prop);
int     JS_HasPropertyStr(JSContext *ctx, JSValueConst obj, const char *prop);

/* ── Array operations ─────────────────────────────────────────── */

int     JS_SetPropertyUint32(JSContext *ctx, JSValue obj, uint32_t idx,
                             JSValue val);
JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx);

/* ── Function creation ────────────────────────────────────────── */

JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name,
                        int length);

/* ── Global object ────────────────────────────────────────────── */

JSValue JS_GetGlobalObject(JSContext *ctx);

/* ── Exception handling ───────────────────────────────────────── */

JSValue JS_GetException(JSContext *ctx);
JSValue JS_Throw(JSContext *ctx, JSValue obj);
JSValue JS_ThrowTypeError(JSContext *ctx, const char *fmt, ...);
JSValue JS_ThrowReferenceError(JSContext *ctx, const char *fmt, ...);

/* ── Pane-specific extensions ─────────────────────────────────── */

/*
 * Set an opaque pointer on the context (used by DOM bridge to store
 * a pointer to the Dart-side callback table).
 */
void  JS_SetContextOpaque(JSContext *ctx, void *opaque);
void *JS_GetContextOpaque(JSContext *ctx);

/*
 * Execution statistics for the last JS_Eval call.
 */
typedef struct {
    int statements_executed;
    int function_calls;
    int exceptions_thrown;
    double elapsed_ms;
} JSExecStats;

int JS_GetExecStats(JSContext *ctx, JSExecStats *stats);

/*
 * Output buffer: accumulates structured output from browser globals
 * (console.log, alert, document operations, navigation).
 * Format: newline-separated "TYPE:data" entries.
 * Call JS_GetOutputBuffer after JS_Eval to read, JS_ClearOutputBuffer to reset.
 */
const char *JS_GetOutputBuffer(JSContext *ctx);
void         JS_ClearOutputBuffer(JSContext *ctx);

/*
 * Install browser globals (console, document, window, alert, etc.)
 * on the context's global object. Output from these functions is
 * written to the output buffer.
 */
void JS_InstallBrowserGlobals(JSContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* QUICKJS_H */
