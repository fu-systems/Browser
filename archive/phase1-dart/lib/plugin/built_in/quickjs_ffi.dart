/// QuickJS FFI Bindings — Dart-side bindings for the native QuickJS engine.
///
/// Wraps the C API from quickjs.h using dart:ffi for type-safe access
/// to the native JavaScript engine.

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

// ── JSValue type (uint64_t in C) ──
typedef JSValue = Uint64;
typedef JSValueDart = int;

// ── Tags (must match quickjs.h) ──
abstract class JsTag {
  static const int undefined = 0;
  static const int null_ = 1;
  static const int bool_ = 2;
  static const int int_ = 3;
  static const int float64 = 4;
  static const int string = 5;
  static const int object = 6;
  static const int function = 7;
  static const int exception = 8;
  static const int array = 9;
  static const int cFunction = 10;
}

/// NaN-boxing helpers matching the C macros.
int jsMkVal(int tag, int payload) =>
    (tag << 48) | (payload & 0xFFFFFFFFFFFF);

int jsValueGetTag(int v) => (v >> 48) & 0xFFFF;

bool jsIsException(int v) => jsValueGetTag(v) == JsTag.exception;
bool jsIsUndefined(int v) => jsValueGetTag(v) == JsTag.undefined;
bool jsIsNull(int v) => jsValueGetTag(v) == JsTag.null_;
bool jsIsString(int v) => jsValueGetTag(v) == JsTag.string;
bool jsIsObject(int v) =>
    jsValueGetTag(v) == JsTag.object || jsValueGetTag(v) == JsTag.array;
bool jsIsFunction(int v) =>
    jsValueGetTag(v) == JsTag.function ||
    jsValueGetTag(v) == JsTag.cFunction;

// ── Predefined JS constants ──
final int jsUndefined = jsMkVal(JsTag.undefined, 0);
final int jsNull = jsMkVal(JsTag.null_, 0);
final int jsTrue = jsMkVal(JsTag.bool_, 1);
final int jsFalse = jsMkVal(JsTag.bool_, 0);
final int jsException = jsMkVal(JsTag.exception, 0);

// ── Opaque pointer types ──
final class JSRuntimePtr extends Opaque {}
final class JSContextPtr extends Opaque {}

// ── JSExecStats struct ──
final class JSExecStats extends Struct {
  @Int32()
  external int statementsExecuted;

  @Int32()
  external int functionCalls;

  @Int32()
  external int exceptionsThrown;

  @Int32()
  external int _padding; // alignment padding for double

  @Double()
  external double elapsedMs;
}

// ── C function callback signature ──
// JSValue (*JSCFunction)(JSContext *ctx, JSValueConst this_val,
//                        int argc, JSValueConst *argv);
typedef JSCFunctionNative = Uint64 Function(
    Pointer<JSContextPtr>, Uint64, Int32, Pointer<Uint64>);
typedef JSCFunctionDart = int Function(
    Pointer<JSContextPtr>, int, int, Pointer<Uint64>);

// ── Native function type signatures ──

// Runtime
typedef _NewRuntimeNative = Pointer<JSRuntimePtr> Function();
typedef _NewRuntimeDart = Pointer<JSRuntimePtr> Function();

typedef _FreeRuntimeNative = Void Function(Pointer<JSRuntimePtr>);
typedef _FreeRuntimeDart = void Function(Pointer<JSRuntimePtr>);

typedef _SetMemoryLimitNative = Void Function(Pointer<JSRuntimePtr>, Size);
typedef _SetMemoryLimitDart = void Function(Pointer<JSRuntimePtr>, int);

typedef _SetMaxStackSizeNative = Void Function(Pointer<JSRuntimePtr>, Size);
typedef _SetMaxStackSizeDart = void Function(Pointer<JSRuntimePtr>, int);

// Interrupt handler
typedef JSInterruptHandlerNative = Int32 Function(
    Pointer<JSRuntimePtr>, Pointer<Void>);
typedef _SetInterruptHandlerNative = Void Function(
    Pointer<JSRuntimePtr>,
    Pointer<NativeFunction<JSInterruptHandlerNative>>,
    Pointer<Void>);
typedef _SetInterruptHandlerDart = void Function(
    Pointer<JSRuntimePtr>,
    Pointer<NativeFunction<JSInterruptHandlerNative>>,
    Pointer<Void>);

// Context
typedef _NewContextNative = Pointer<JSContextPtr> Function(
    Pointer<JSRuntimePtr>);
typedef _NewContextDart = Pointer<JSContextPtr> Function(
    Pointer<JSRuntimePtr>);

typedef _FreeContextNative = Void Function(Pointer<JSContextPtr>);
typedef _FreeContextDart = void Function(Pointer<JSContextPtr>);

typedef _GetRuntimeNative = Pointer<JSRuntimePtr> Function(
    Pointer<JSContextPtr>);
typedef _GetRuntimeDart = Pointer<JSRuntimePtr> Function(
    Pointer<JSContextPtr>);

// Eval
typedef _EvalNative = Uint64 Function(
    Pointer<JSContextPtr>, Pointer<Utf8>, Size, Pointer<Utf8>, Int32);
typedef _EvalDart = int Function(
    Pointer<JSContextPtr>, Pointer<Utf8>, int, Pointer<Utf8>, int);

// Value creation
typedef _NewStringNative = Uint64 Function(
    Pointer<JSContextPtr>, Pointer<Utf8>);
typedef _NewStringDart = int Function(
    Pointer<JSContextPtr>, Pointer<Utf8>);

typedef _NewStringLenNative = Uint64 Function(
    Pointer<JSContextPtr>, Pointer<Utf8>, Size);
typedef _NewStringLenDart = int Function(
    Pointer<JSContextPtr>, Pointer<Utf8>, int);

typedef _NewInt32Native = Uint64 Function(Pointer<JSContextPtr>, Int32);
typedef _NewInt32Dart = int Function(Pointer<JSContextPtr>, int);

typedef _NewFloat64Native = Uint64 Function(Pointer<JSContextPtr>, Double);
typedef _NewFloat64Dart = int Function(Pointer<JSContextPtr>, double);

typedef _NewBoolNative = Uint64 Function(Pointer<JSContextPtr>, Int32);
typedef _NewBoolDart = int Function(Pointer<JSContextPtr>, int);

typedef _NewObjectNative = Uint64 Function(Pointer<JSContextPtr>);
typedef _NewObjectDart = int Function(Pointer<JSContextPtr>);

typedef _NewArrayNative = Uint64 Function(Pointer<JSContextPtr>);
typedef _NewArrayDart = int Function(Pointer<JSContextPtr>);

// Value extraction
typedef _ToCStringNative = Pointer<Utf8> Function(
    Pointer<JSContextPtr>, Uint64);
typedef _ToCStringDart = Pointer<Utf8> Function(
    Pointer<JSContextPtr>, int);

typedef _FreeCStringNative = Void Function(
    Pointer<JSContextPtr>, Pointer<Utf8>);
typedef _FreeCStringDart = void Function(
    Pointer<JSContextPtr>, Pointer<Utf8>);

typedef _ToInt32Native = Int32 Function(
    Pointer<JSContextPtr>, Pointer<Int32>, Uint64);
typedef _ToInt32Dart = int Function(
    Pointer<JSContextPtr>, Pointer<Int32>, int);

typedef _ToFloat64Native = Int32 Function(
    Pointer<JSContextPtr>, Pointer<Double>, Uint64);
typedef _ToFloat64Dart = int Function(
    Pointer<JSContextPtr>, Pointer<Double>, int);

typedef _ToBoolNative = Int32 Function(Pointer<JSContextPtr>, Uint64);
typedef _ToBoolDart = int Function(Pointer<JSContextPtr>, int);

// Reference counting
typedef _DupValueNative = Uint64 Function(Pointer<JSContextPtr>, Uint64);
typedef _DupValueDart = int Function(Pointer<JSContextPtr>, int);

typedef _FreeValueNative = Void Function(Pointer<JSContextPtr>, Uint64);
typedef _FreeValueDart = void Function(Pointer<JSContextPtr>, int);

// Property access
typedef _GetPropertyStrNative = Uint64 Function(
    Pointer<JSContextPtr>, Uint64, Pointer<Utf8>);
typedef _GetPropertyStrDart = int Function(
    Pointer<JSContextPtr>, int, Pointer<Utf8>);

typedef _SetPropertyStrNative = Int32 Function(
    Pointer<JSContextPtr>, Uint64, Pointer<Utf8>, Uint64);
typedef _SetPropertyStrDart = int Function(
    Pointer<JSContextPtr>, int, Pointer<Utf8>, int);

typedef _DeletePropertyStrNative = Int32 Function(
    Pointer<JSContextPtr>, Uint64, Pointer<Utf8>);
typedef _DeletePropertyStrDart = int Function(
    Pointer<JSContextPtr>, int, Pointer<Utf8>);

typedef _HasPropertyStrNative = Int32 Function(
    Pointer<JSContextPtr>, Uint64, Pointer<Utf8>);
typedef _HasPropertyStrDart = int Function(
    Pointer<JSContextPtr>, int, Pointer<Utf8>);

// Array operations
typedef _SetPropertyUint32Native = Int32 Function(
    Pointer<JSContextPtr>, Uint64, Uint32, Uint64);
typedef _SetPropertyUint32Dart = int Function(
    Pointer<JSContextPtr>, int, int, int);

typedef _GetPropertyUint32Native = Uint64 Function(
    Pointer<JSContextPtr>, Uint64, Uint32);
typedef _GetPropertyUint32Dart = int Function(
    Pointer<JSContextPtr>, int, int);

// Function creation
typedef _NewCFunctionNative = Uint64 Function(
    Pointer<JSContextPtr>,
    Pointer<NativeFunction<JSCFunctionNative>>,
    Pointer<Utf8>,
    Int32);
typedef _NewCFunctionDart = int Function(
    Pointer<JSContextPtr>,
    Pointer<NativeFunction<JSCFunctionNative>>,
    Pointer<Utf8>,
    int);

// Global object
typedef _GetGlobalObjectNative = Uint64 Function(Pointer<JSContextPtr>);
typedef _GetGlobalObjectDart = int Function(Pointer<JSContextPtr>);

// Exception handling
typedef _GetExceptionNative = Uint64 Function(Pointer<JSContextPtr>);
typedef _GetExceptionDart = int Function(Pointer<JSContextPtr>);

typedef _ThrowNative = Uint64 Function(Pointer<JSContextPtr>, Uint64);
typedef _ThrowDart = int Function(Pointer<JSContextPtr>, int);

// Opaque pointer
typedef _SetContextOpaqueNative = Void Function(
    Pointer<JSContextPtr>, Pointer<Void>);
typedef _SetContextOpaqueDart = void Function(
    Pointer<JSContextPtr>, Pointer<Void>);

typedef _GetContextOpaqueNative = Pointer<Void> Function(
    Pointer<JSContextPtr>);
typedef _GetContextOpaqueDart = Pointer<Void> Function(
    Pointer<JSContextPtr>);

// Exec stats
typedef _GetExecStatsNative = Int32 Function(
    Pointer<JSContextPtr>, Pointer<JSExecStats>);
typedef _GetExecStatsDart = int Function(
    Pointer<JSContextPtr>, Pointer<JSExecStats>);

// Output buffer
typedef _GetOutputBufferNative = Pointer<Utf8> Function(Pointer<JSContextPtr>);
typedef _GetOutputBufferDart = Pointer<Utf8> Function(Pointer<JSContextPtr>);

typedef _ClearOutputBufferNative = Void Function(Pointer<JSContextPtr>);
typedef _ClearOutputBufferDart = void Function(Pointer<JSContextPtr>);

// Browser globals installation
typedef _InstallBrowserGlobalsNative = Void Function(Pointer<JSContextPtr>);
typedef _InstallBrowserGlobalsDart = void Function(Pointer<JSContextPtr>);

// ── QuickJS bindings class ──

/// Dart FFI bindings to the native QuickJS engine.
///
/// Call [QuickJSBindings.instance] to get the singleton.
class QuickJSBindings {
  static QuickJSBindings? _instance;
  final DynamicLibrary _lib;

  // Runtime
  late final _NewRuntimeDart newRuntime;
  late final _FreeRuntimeDart freeRuntime;
  late final _SetMemoryLimitDart setMemoryLimit;
  late final _SetMaxStackSizeDart setMaxStackSize;
  late final _SetInterruptHandlerDart setInterruptHandler;

  // Context
  late final _NewContextDart newContext;
  late final _FreeContextDart freeContext;
  late final _GetRuntimeDart getRuntime;

  // Eval
  late final _EvalDart eval;

  // Value creation
  late final _NewStringDart newString;
  late final _NewStringLenDart newStringLen;
  late final _NewInt32Dart newInt32;
  late final _NewFloat64Dart newFloat64;
  late final _NewBoolDart newBool;
  late final _NewObjectDart newObject;
  late final _NewArrayDart newArray;

  // Value extraction
  late final _ToCStringDart toCString;
  late final _FreeCStringDart freeCString;
  late final _ToInt32Dart toInt32;
  late final _ToFloat64Dart toFloat64;
  late final _ToBoolDart toBool;

  // Reference counting
  late final _DupValueDart dupValue;
  late final _FreeValueDart freeValue;

  // Property access
  late final _GetPropertyStrDart getPropertyStr;
  late final _SetPropertyStrDart setPropertyStr;
  late final _DeletePropertyStrDart deletePropertyStr;
  late final _HasPropertyStrDart hasPropertyStr;

  // Array
  late final _SetPropertyUint32Dart setPropertyUint32;
  late final _GetPropertyUint32Dart getPropertyUint32;

  // Function creation
  late final _NewCFunctionDart newCFunction;

  // Global
  late final _GetGlobalObjectDart getGlobalObject;

  // Exceptions
  late final _GetExceptionDart getException;
  late final _ThrowDart jsThrow;

  // Opaque
  late final _SetContextOpaqueDart setContextOpaque;
  late final _GetContextOpaqueDart getContextOpaque;

  // Stats
  late final _GetExecStatsDart getExecStats;

  // Output buffer
  late final _GetOutputBufferDart getOutputBuffer;
  late final _ClearOutputBufferDart clearOutputBuffer;

  // Browser globals
  late final _InstallBrowserGlobalsDart installBrowserGlobals;

  QuickJSBindings._(this._lib) {
    // Runtime
    newRuntime = _lib
        .lookupFunction<_NewRuntimeNative, _NewRuntimeDart>('JS_NewRuntime');
    freeRuntime = _lib
        .lookupFunction<_FreeRuntimeNative, _FreeRuntimeDart>('JS_FreeRuntime');
    setMemoryLimit = _lib.lookupFunction<_SetMemoryLimitNative,
        _SetMemoryLimitDart>('JS_SetMemoryLimit');
    setMaxStackSize = _lib.lookupFunction<_SetMaxStackSizeNative,
        _SetMaxStackSizeDart>('JS_SetMaxStackSize');
    setInterruptHandler = _lib.lookupFunction<_SetInterruptHandlerNative,
        _SetInterruptHandlerDart>('JS_SetInterruptHandler');

    // Context
    newContext = _lib
        .lookupFunction<_NewContextNative, _NewContextDart>('JS_NewContext');
    freeContext = _lib
        .lookupFunction<_FreeContextNative, _FreeContextDart>('JS_FreeContext');
    getRuntime = _lib
        .lookupFunction<_GetRuntimeNative, _GetRuntimeDart>('JS_GetRuntime');

    // Eval
    eval = _lib.lookupFunction<_EvalNative, _EvalDart>('JS_Eval');

    // Value creation
    newString = _lib
        .lookupFunction<_NewStringNative, _NewStringDart>('JS_NewString');
    newStringLen = _lib.lookupFunction<_NewStringLenNative, _NewStringLenDart>(
        'JS_NewStringLen');
    newInt32 = _lib
        .lookupFunction<_NewInt32Native, _NewInt32Dart>('JS_NewInt32');
    newFloat64 = _lib
        .lookupFunction<_NewFloat64Native, _NewFloat64Dart>('JS_NewFloat64');
    newBool =
        _lib.lookupFunction<_NewBoolNative, _NewBoolDart>('JS_NewBool');
    newObject = _lib
        .lookupFunction<_NewObjectNative, _NewObjectDart>('JS_NewObject');
    newArray =
        _lib.lookupFunction<_NewArrayNative, _NewArrayDart>('JS_NewArray');

    // Value extraction
    toCString = _lib
        .lookupFunction<_ToCStringNative, _ToCStringDart>('JS_ToCString');
    freeCString = _lib.lookupFunction<_FreeCStringNative, _FreeCStringDart>(
        'JS_FreeCString');
    toInt32 =
        _lib.lookupFunction<_ToInt32Native, _ToInt32Dart>('JS_ToInt32');
    toFloat64 = _lib
        .lookupFunction<_ToFloat64Native, _ToFloat64Dart>('JS_ToFloat64');
    toBool =
        _lib.lookupFunction<_ToBoolNative, _ToBoolDart>('JS_ToBool');

    // Reference counting
    dupValue = _lib
        .lookupFunction<_DupValueNative, _DupValueDart>('JS_DupValue');
    freeValue = _lib
        .lookupFunction<_FreeValueNative, _FreeValueDart>('JS_FreeValue');

    // Property access
    getPropertyStr = _lib.lookupFunction<_GetPropertyStrNative,
        _GetPropertyStrDart>('JS_GetPropertyStr');
    setPropertyStr = _lib.lookupFunction<_SetPropertyStrNative,
        _SetPropertyStrDart>('JS_SetPropertyStr');
    deletePropertyStr = _lib.lookupFunction<_DeletePropertyStrNative,
        _DeletePropertyStrDart>('JS_DeletePropertyStr');
    hasPropertyStr = _lib.lookupFunction<_HasPropertyStrNative,
        _HasPropertyStrDart>('JS_HasPropertyStr');

    // Array
    setPropertyUint32 = _lib.lookupFunction<_SetPropertyUint32Native,
        _SetPropertyUint32Dart>('JS_SetPropertyUint32');
    getPropertyUint32 = _lib.lookupFunction<_GetPropertyUint32Native,
        _GetPropertyUint32Dart>('JS_GetPropertyUint32');

    // Function creation
    newCFunction = _lib.lookupFunction<_NewCFunctionNative,
        _NewCFunctionDart>('JS_NewCFunction');

    // Global
    getGlobalObject = _lib.lookupFunction<_GetGlobalObjectNative,
        _GetGlobalObjectDart>('JS_GetGlobalObject');

    // Exceptions
    getException = _lib.lookupFunction<_GetExceptionNative,
        _GetExceptionDart>('JS_GetException');
    jsThrow =
        _lib.lookupFunction<_ThrowNative, _ThrowDart>('JS_Throw');

    // Opaque
    setContextOpaque = _lib.lookupFunction<_SetContextOpaqueNative,
        _SetContextOpaqueDart>('JS_SetContextOpaque');
    getContextOpaque = _lib.lookupFunction<_GetContextOpaqueNative,
        _GetContextOpaqueDart>('JS_GetContextOpaque');

    // Stats
    getExecStats = _lib.lookupFunction<_GetExecStatsNative,
        _GetExecStatsDart>('JS_GetExecStats');

    // Output buffer
    getOutputBuffer = _lib.lookupFunction<_GetOutputBufferNative,
        _GetOutputBufferDart>('JS_GetOutputBuffer');
    clearOutputBuffer = _lib.lookupFunction<_ClearOutputBufferNative,
        _ClearOutputBufferDart>('JS_ClearOutputBuffer');

    // Browser globals
    installBrowserGlobals = _lib.lookupFunction<_InstallBrowserGlobalsNative,
        _InstallBrowserGlobalsDart>('JS_InstallBrowserGlobals');
  }

  /// Load the native QuickJS library and create bindings.
  static QuickJSBindings get instance {
    if (_instance != null) return _instance!;

    DynamicLibrary lib;
    final libName = Platform.isLinux ? 'libquickjs.so' : 'libquickjs.dylib';

    try {
      // Try loading from standard library path
      lib = DynamicLibrary.open(libName);
    } catch (_) {
      // Try loading from the native directory (development)
      try {
        lib = DynamicLibrary.open('native/quickjs/$libName');
      } catch (_) {
        // Try relative to executable
        final exeDir = File(Platform.resolvedExecutable).parent.path;
        lib = DynamicLibrary.open('$exeDir/lib/$libName');
      }
    }

    _instance = QuickJSBindings._(lib);
    return _instance!;
  }
}
