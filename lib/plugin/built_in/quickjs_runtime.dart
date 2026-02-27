/// QuickJS Runtime — sandboxed JavaScript execution with output buffer protocol.
///
/// Manages the native QuickJS context lifecycle. Browser globals (console,
/// document, window, alert) are installed in C and write structured output
/// to a buffer. After each eval, the Dart layer reads and parses the buffer
/// to build a [JsExecResult].

import 'dart:convert';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../engine/dom.dart';
import 'quickjs_ffi.dart';

/// Result of executing JavaScript source code.
class JsExecResult {
  /// Console output captured during execution.
  final List<String> log;

  /// Alert messages captured.
  final List<String> alerts;

  /// Pending navigation from window.location assignment.
  final String? pendingNavigation;

  /// Whether the DOM was modified.
  final bool domModified;

  /// Statements that raised exceptions.
  final List<String> errors;

  /// Execution statistics.
  final double elapsedMs;
  final int statementsExecuted;

  const JsExecResult({
    this.log = const [],
    this.alerts = const [],
    this.pendingNavigation,
    this.domModified = false,
    this.errors = const [],
    this.elapsedMs = 0,
    this.statementsExecuted = 0,
  });
}

/// Sandboxed JavaScript runtime backed by native QuickJS.
///
/// Each instance creates its own QuickJS runtime and context with:
/// - Memory limit (default 8 MB)
/// - Stack size limit (default 256 KB)
/// - Injected browser globals (console, document, window, alert)
///
/// The C engine handles all JS execution and browser global calls.
/// Output is communicated via a structured output buffer read after eval.
class QuickJSRuntime {
  static const int _defaultMemoryLimit = 8 * 1024 * 1024; // 8 MB
  static const int _defaultMaxStackSize = 256 * 1024; // 256 KB

  QuickJSBindings? _bindings;
  Pointer<JSRuntimePtr>? _runtime;
  Pointer<JSContextPtr>? _context;
  bool _disposed = false;
  bool _initialized = false;

  /// Whether this runtime has been disposed.
  bool get isDisposed => _disposed;

  /// Try to initialize the native engine. Returns false if the library
  /// cannot be loaded (e.g., not built yet).
  bool initialize() {
    if (_initialized) return true;
    if (_disposed) return false;

    try {
      _bindings = QuickJSBindings.instance;
      _runtime = _bindings!.newRuntime();
      _bindings!.setMemoryLimit(_runtime!, _defaultMemoryLimit);
      _bindings!.setMaxStackSize(_runtime!, _defaultMaxStackSize);
      _context = _bindings!.newContext(_runtime!);
      _bindings!.installBrowserGlobals(_context!);
      _initialized = true;
      return true;
    } catch (e) {
      _disposed = true;
      return false;
    }
  }

  /// Execute JavaScript source and return results.
  ///
  /// The [document] parameter provides DOM context for document operations.
  /// If [blockTransmit] is true, navigation and network requests are blocked.
  JsExecResult eval(String source, Document document,
      {bool blockTransmit = false}) {
    if (_disposed || !_initialized || _bindings == null) {
      return const JsExecResult(errors: ['Runtime not initialized']);
    }

    final b = _bindings!;
    final ctx = _context!;

    // Clear output buffer before eval
    b.clearOutputBuffer(ctx);

    // Use UTF-8 byte length, not Dart string length (UTF-16 code units)
    final sourceBytes = utf8.encode(source);
    final sourcePtr = source.toNativeUtf8(allocator: malloc);
    final filenamePtr = '<script>'.toNativeUtf8(allocator: malloc);

    final errors = <String>[];

    try {
      final result = b.eval(
        ctx,
        sourcePtr.cast(),
        sourceBytes.length,
        filenamePtr.cast(),
        0, // JS_EVAL_TYPE_GLOBAL
      );

      // Check for exception
      if (jsIsException(result)) {
        final exc = b.getException(ctx);
        final excStr = _toCString(ctx, exc);
        errors.add(excStr ?? 'Unknown JS exception');
        b.freeValue(ctx, exc);
      } else {
        b.freeValue(ctx, result);
      }
    } catch (e) {
      errors.add('FFI error: $e');
    } finally {
      malloc.free(sourcePtr);
      malloc.free(filenamePtr);
    }

    // Read output buffer
    final outputPtr = b.getOutputBuffer(ctx);
    final output = outputPtr.address != 0
        ? outputPtr.cast<Utf8>().toDartString()
        : '';

    // Parse structured output
    final parsed = _parseOutputBuffer(output, document, blockTransmit);

    // Get execution stats
    double elapsed = 0;
    int stmts = 0;
    final stats = calloc<JSExecStats>();
    try {
      if (b.getExecStats(ctx, stats) == 0) {
        elapsed = stats.ref.elapsedMs;
        stmts = stats.ref.statementsExecuted;
      }
    } finally {
      calloc.free(stats);
    }

    return JsExecResult(
      log: List.unmodifiable(parsed.log),
      alerts: List.unmodifiable(parsed.alerts),
      pendingNavigation: parsed.pendingNavigation,
      domModified: parsed.domModified,
      errors: List.unmodifiable([...errors, ...parsed.errors]),
      elapsedMs: elapsed,
      statementsExecuted: stmts,
    );
  }

  /// Release native resources.
  void dispose() {
    if (_disposed) return;
    _disposed = true;
    if (_initialized && _bindings != null) {
      _bindings!.freeContext(_context!);
      _bindings!.freeRuntime(_runtime!);
    }
  }

  // ── Output buffer parsing ──────────────────────────────────

  _ParsedOutput _parseOutputBuffer(
      String buffer, Document document, bool blockTransmit) {
    final log = <String>[];
    final alerts = <String>[];
    final errors = <String>[];
    String? pendingNav;
    bool domModified = false;

    if (buffer.isEmpty) {
      return _ParsedOutput(
          log: log,
          alerts: alerts,
          errors: errors,
          pendingNavigation: pendingNav,
          domModified: domModified);
    }

    for (final line in buffer.split('\n')) {
      if (line.isEmpty) continue;

      final colonIdx = line.indexOf(':');
      if (colonIdx < 0) continue;

      final type = line.substring(0, colonIdx);
      final data = line.substring(colonIdx + 1);

      switch (type) {
        case 'LOG':
          log.add('[console.log] $data');
          break;
        case 'WARN':
          log.add('[console.warn] $data');
          break;
        case 'ERROR':
          log.add('[console.error] $data');
          break;
        case 'ALERT':
          alerts.add(data);
          log.add('[engine] alert("${_truncate(data, 60)}")');
          break;
        case 'NAV':
          if (blockTransmit) {
            log.add('[blocked] navigation to $data (outbound blocked)');
          } else {
            pendingNav = data;
            log.add('[engine] window.location = "$data"');
          }
          break;
        case 'DOM_WRITE':
          _applyDocumentWrite(document, data);
          domModified = true;
          log.add('[engine] document.write: ${_truncate(data, 80)}');
          break;
        case 'DOM_QUERY':
          log.add('[engine] DOM query: $data');
          break;
        case 'DOM_CREATE':
          domModified = true;
          log.add('[engine] DOM create: $data');
          break;
        case 'COOKIE':
          if (blockTransmit) {
            log.add('[blocked] document.cookie write (outbound blocked)');
          } else {
            log.add('[engine] document.cookie = "${_truncate(data, 60)}"');
          }
          break;
        case 'FETCH':
          if (blockTransmit) {
            log.add('[blocked] network request to $data (outbound blocked)');
          } else {
            log.add('[engine] fetch: $data');
          }
          break;
        case 'TIMER':
          log.add('[engine] $data (no-op)');
          break;
        case 'EVENT':
          log.add('[engine] event: $data');
          break;
        default:
          log.add('[engine] $type: $data');
          break;
      }
    }

    return _ParsedOutput(
        log: log,
        alerts: alerts,
        errors: errors,
        pendingNavigation: pendingNav,
        domModified: domModified);
  }

  void _applyDocumentWrite(Document document, String html) {
    final body = document.body;
    if (body != null) {
      body.appendChild(Text(html));
    }
  }

  String? _toCString(Pointer<JSContextPtr> ctx, int jsVal) {
    final ptr = _bindings!.toCString(ctx, jsVal);
    if (ptr == nullptr) return null;
    final str = ptr.cast<Utf8>().toDartString();
    _bindings!.freeCString(ctx, ptr.cast());
    return str;
  }

  String _truncate(String s, int max) =>
      s.length <= max ? s : '${s.substring(0, max)}...';
}

class _ParsedOutput {
  final List<String> log;
  final List<String> alerts;
  final List<String> errors;
  final String? pendingNavigation;
  final bool domModified;

  _ParsedOutput({
    required this.log,
    required this.alerts,
    required this.errors,
    required this.pendingNavigation,
    required this.domModified,
  });
}
