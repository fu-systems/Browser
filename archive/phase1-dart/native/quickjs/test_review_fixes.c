/* Test cases for review bug fixes */
#include "quickjs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int pass = 0, fail = 0;

static void test_eval(const char *desc, const char *code, int expected_int) {
    JSRuntime *rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 8*1024*1024);
    JSContext *ctx = JS_NewContext(rt);
    JS_InstallBrowserGlobals(ctx);

    JSValue result = JS_Eval(ctx, code, strlen(code), "<test>", 0);
    int tag = JS_VALUE_GET_TAG(result);
    int val = JS_VALUE_GET_INT(result);

    /* Accept INT, BOOL, or FLOAT64 as long as value matches.
     * Note: 0 is stored as FLOAT64 by design (to avoid -0 confusion). */
    int passed = 0;
    if (tag == JS_TAG_INT && val == expected_int) {
        passed = 1;
    } else if (tag == JS_TAG_BOOL && val == expected_int) {
        passed = 1;
    } else if (tag == JS_TAG_FLOAT64) {
        double d;
        JS_ToFloat64(ctx, &d, result);
        passed = (d == (double)expected_int);
    }

    if (passed) {
        printf("  PASS: %s\n", desc);
        pass++;
    } else {
        printf("  FAIL: %s (got tag=%d val=%d, expected %d)\n", desc, tag, val, expected_int);
        fail++;
    }

    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

static void test_eval_str(const char *desc, const char *code, const char *expected) {
    JSRuntime *rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 8*1024*1024);
    JSContext *ctx = JS_NewContext(rt);
    JS_InstallBrowserGlobals(ctx);
    
    JSValue result = JS_Eval(ctx, code, strlen(code), "<test>", 0);
    const char *s = JS_ToCString(ctx, result);
    
    int passed = s && strcmp(s, expected) == 0;
    if (passed) {
        printf("  PASS: %s\n", desc);
        pass++;
    } else {
        printf("  FAIL: %s (got '%s', expected '%s')\n", desc, s ? s : "(null)", expected);
        fail++;
    }
    
    if (s) JS_FreeCString(ctx, s);
    JS_FreeValue(ctx, result);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

int main(void) {
    printf("=== Ternary: skip not-taken branch ===\n");
    test_eval("true ? 1 : 0", "true ? 1 : 0", 1);
    test_eval("false ? 1 : 0", "false ? 1 : 0", 0);
    test_eval("5 > 3 ? 10 : 20", "5 > 3 ? 10 : 20", 10);
    test_eval("3 > 5 ? 10 : 20", "3 > 5 ? 10 : 20", 20);
    
    printf("\n=== If/else: skip not-taken branch ===\n");
    test_eval("if true block", "var x = 0; if (true) { x = 1; } else { x = 2; } x", 1);
    test_eval("if false block", "var x = 0; if (false) { x = 1; } else { x = 2; } x", 2);
    test_eval("if-else-if (first true)",
        "var x = 0; if (true) { x = 1; } else if (true) { x = 2; } else { x = 3; } x", 1);
    test_eval("if-else-if (second true)",
        "var x = 0; if (false) { x = 1; } else if (true) { x = 2; } else { x = 3; } x", 2);
    test_eval("if-else-if (none true)",
        "var x = 0; if (false) { x = 1; } else if (false) { x = 2; } else { x = 3; } x", 3);
    
    printf("\n=== Short-circuit: && || ?? ===\n");
    test_eval("false && x (short-circuit)", "var x = 10; false && x", 0);
    test_eval("true || x (short-circuit)", "var x = 10; true || x", 1);
    test_eval("1 && 2", "1 && 2", 2);
    test_eval("0 || 5", "0 || 5", 5);
    
    printf("\n=== Return from function ===\n");
    test_eval("simple return",
        "function f() { return 42; } f()", 42);
    test_eval("nested return",
        "function inner() { return 10; } function outer() { return inner() + 5; } outer()", 15);
    test_eval("return in try",
        "function f() { try { return 99; } catch(e) {} } f()", 99);
    
    printf("\n=== Delete operator (no leak) ===\n");
    test_eval("delete var", "var x = 5; delete x; 1", 1);
    
    printf("\n=== Multi-param function (comma fix) ===\n");
    test_eval("f(a,b) return a", "function f(a,b) { return a; } f(11,22)", 11);
    test_eval("f(a,b) return b", "function f(a,b) { return b; } f(11,22)", 22);
    test_eval("f(a,b,c) sum", "function f(a,b,c) { return a+b+c; } f(1,2,3)", 6);
    
    printf("\n=== Output buffer newline sanitization ===\n");
    {
        JSRuntime *rt = JS_NewRuntime();
        JS_SetMemoryLimit(rt, 8*1024*1024);
        JSContext *ctx = JS_NewContext(rt);
        JS_InstallBrowserGlobals(ctx);
        
        JS_Eval(ctx, "console.log('line1\\nline2')", 26, "<test>", 0);
        const char *buf = JS_GetOutputBuffer(ctx);
        /* Newlines in data should be sanitized to spaces */
        int has_log = strstr(buf, "LOG:") != NULL;
        /* The output should be a single LOG entry, not two lines that look like LOG entries */
        int only_one_log = 1;
        const char *p = buf;
        int count = 0;
        while ((p = strstr(p, "LOG:")) != NULL) { count++; p++; }
        only_one_log = (count == 1);
        
        if (has_log && only_one_log) {
            printf("  PASS: newline in console.log sanitized\n");
            pass++;
        } else {
            printf("  FAIL: newline in console.log (buf='%s', count=%d)\n", buf, count);
            fail++;
        }
        
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }
    
    printf("\n===================================\n");
    printf("Results: %d passed, %d failed, %d total\n", pass, fail, pass + fail);
    return fail > 0 ? 1 : 0;
}
