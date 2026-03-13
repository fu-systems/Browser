#include "quickjs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

static void check_int(const char *name, JSContext *ctx, const char *code, int expected) {
    JSValue r = JS_Eval(ctx, code, strlen(code), "<test>", 0);
    int tag = JS_VALUE_GET_TAG(r);
    int val = JS_VALUE_GET_INT(r);
    if (tag == JS_TAG_INT && val == expected) {
        printf("  PASS: %s => %d\n", name, val);
        tests_passed++;
    } else {
        printf("  FAIL: %s => tag=%d val=%d (expected %d)\n", name, tag, val, expected);
        tests_failed++;
    }
    JS_FreeValue(ctx, r);
}

static void check_str(const char *name, JSContext *ctx, const char *code, const char *expected) {
    JSValue r = JS_Eval(ctx, code, strlen(code), "<test>", 0);
    const char *got = JS_ToCString(ctx, r);
    if (got && strcmp(got, expected) == 0) {
        printf("  PASS: %s => \"%s\"\n", name, got);
        tests_passed++;
    } else {
        printf("  FAIL: %s => \"%s\" (expected \"%s\")\n", name, got ? got : "(null)", expected);
        tests_failed++;
    }
    if (got) free((void*)got);
    JS_FreeValue(ctx, r);
}

static void check_output(const char *name, JSContext *ctx, const char *code, const char *expected_substr) {
    JS_ClearOutputBuffer(ctx);
    JSValue r = JS_Eval(ctx, code, strlen(code), "<test>", 0);
    const char *buf = JS_GetOutputBuffer(ctx);
    if (buf && strstr(buf, expected_substr)) {
        printf("  PASS: %s (output contains \"%s\")\n", name, expected_substr);
        tests_passed++;
    } else {
        printf("  FAIL: %s output=\"%s\" (expected to contain \"%s\")\n",
               name, buf ? buf : "(null)", expected_substr);
        tests_failed++;
    }
    JS_FreeValue(ctx, r);
}

int main(void) {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    JS_InstallBrowserGlobals(ctx);

    printf("=== Arithmetic ===\n");
    check_int("2+3", ctx, "2+3;", 5);
    check_int("10-4", ctx, "10-4;", 6);
    check_int("3*7", ctx, "3*7;", 21);

    printf("\n=== Variables ===\n");
    check_int("var x=5; x", ctx, "var x = 5; x;", 5);
    check_int("var a=3,b=4; a+b", ctx, "var a = 3; var b = 4; a+b;", 7);

    printf("\n=== String concat ===\n");
    check_str("hello+world", ctx, "'hello' + ' ' + 'world';", "hello world");

    printf("\n=== Functions ===\n");
    check_int("single param", ctx, "function f(x) { return x; } f(42);", 42);
    check_int("two params return first", ctx, "function g(a, b) { return a; } g(11, 22);", 11);
    check_int("two params return second", ctx, "function h(a, b) { return b; } h(11, 22);", 22);
    check_int("two params sum", ctx, "function add(a, b) { return a + b; } add(3, 4);", 7);
    check_int("three params", ctx, "function f3(a,b,c) { return b; } f3(10,20,30);", 20);
    check_int("function var capture", ctx,
        "function fv(a,b) { var x = a; return x; } fv(55,66);", 55);
    check_int("nested calls", ctx,
        "function dbl(x) { return x + x; } dbl(dbl(3));", 12);
    check_int("recursive factorial", ctx,
        "function fact(n) { if (n <= 1) return 1; return n * fact(n - 1); } fact(5);", 120);

    printf("\n=== Control Flow ===\n");
    check_int("if true", ctx, "var r1 = 0; if (true) r1 = 1; r1;", 1);
    check_int("if false else", ctx, "var r2 = 0; if (false) r2 = 1; else r2 = 2; r2;", 2);
    check_int("for loop sum", ctx,
        "var s = 0; for (var i = 1; i <= 10; i++) { s = s + i; } s;", 55);
    check_int("while loop", ctx,
        "var w = 0; while (w < 5) { w = w + 1; } w;", 5);

    printf("\n=== Objects ===\n");
    check_int("object property", ctx,
        "var obj1 = { x: 10, y: 20 }; obj1.x + obj1.y;", 30);
    check_int("object bracket", ctx,
        "var obj2 = { name: 42 }; obj2['name'];", 42);

    printf("\n=== Arrays ===\n");
    check_int("array length", ctx,
        "var arr1 = [1, 2, 3]; arr1.length;", 3);
    check_int("array index", ctx,
        "var arr2 = [10, 20, 30]; arr2[1];", 20);

    printf("\n=== Try/Catch ===\n");
    check_int("try/catch", ctx,
        "var tc = 0; try { throw 42; } catch(e) { tc = e; } tc;", 42);

    printf("\n=== Console/Alert ===\n");
    check_output("console.log", ctx, "console.log('hello');", "LOG:hello");
    check_output("alert", ctx, "alert('test');", "ALERT:test");

    printf("\n=== String Methods ===\n");
    check_int("indexOf", ctx, "'hello world'.indexOf('world');", 6);
    check_int("length", ctx, "'hello'.length;", 5);
    check_str("toUpperCase", ctx, "'hello'.toUpperCase();", "HELLO");
    check_str("toLowerCase", ctx, "'HELLO'.toLowerCase();", "hello");
    check_str("substring", ctx, "'hello world'.substring(0, 5);", "hello");
    check_str("trim", ctx, "'  hello  '.trim();", "hello");

    printf("\n=== Comparison ===\n");
    check_int("5 > 3", ctx, "5 > 3 ? 1 : 0;", 1);
    check_int("3 < 5", ctx, "3 < 5 ? 1 : 0;", 1);
    check_int("5 == 5", ctx, "5 == 5 ? 1 : 0;", 1);
    check_int("5 != 3", ctx, "5 != 3 ? 1 : 0;", 1);

    printf("\n=== Switch ===\n");
    check_int("switch", ctx,
        "var sw = 2; var sr = 0; switch(sw) { case 1: sr = 10; break; case 2: sr = 20; break; default: sr = 30; } sr;", 20);

    printf("\n=== Logical ===\n");
    check_int("and true", ctx, "true && true ? 1 : 0;", 1);
    check_int("or false", ctx, "false || true ? 1 : 0;", 1);
    check_int("not", ctx, "!false ? 1 : 0;", 1);

    printf("\n===================================\n");
    printf("Results: %d passed, %d failed, %d total\n",
           tests_passed, tests_failed, tests_passed + tests_failed);

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return tests_failed > 0 ? 1 : 0;
}
