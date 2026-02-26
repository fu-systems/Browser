/// Script Manager — built-in plugin for JavaScript execution control.
///
/// Three execution modes per tab: off (default), run all, or ask for
/// each script. When JS is enabled, outbound data transmission can be
/// independently blocked. Extracts scripts from the DOM during
/// [onDomReady] and executes them via [ScriptEngine].

import '../../engine/dom.dart';
import '../../network/logger.dart';
import '../plugin.dart';
import 'script_engine.dart';

// ── Enums ───────────────────────────────────────────────────────

enum ScriptMode {
  off('OFF'),
  runAll('ALL'),
  askEach('ASK');

  final String label;
  const ScriptMode(this.label);
}

enum TransmitMode {
  normal('Normal'),
  blocked('Blocked');

  final String label;
  const TransmitMode(this.label);
}

// ── Script metadata ─────────────────────────────────────────────

/// Information about a single script found on the page.
class ScriptInfo {
  /// External URL, or `null` for inline scripts.
  final String? src;

  /// Script content (inline body, or empty for external).
  final String content;

  /// Whether this is an inline <script> (vs external src).
  bool get isInline => src == null;

  /// First N chars of content for preview.
  String get preview {
    if (content.isEmpty) return src ?? '(empty)';
    final clean = content.trim().replaceAll(RegExp(r'\s+'), ' ');
    return clean.length <= 120 ? clean : '${clean.substring(0, 120)}...';
  }

  /// Byte size for display.
  int get sizeBytes => content.length;

  /// Whether the user approved this script (for ask-each mode).
  bool approved;

  ScriptInfo({
    this.src,
    required this.content,
    this.approved = false,
  });
}

// ── Plugin ──────────────────────────────────────────────────────

class ScriptManagerPlugin extends Plugin {
  @override
  String get name => 'script_manager';

  // ── State set by browser shell before each load ──
  ScriptMode currentMode = ScriptMode.off;
  TransmitMode currentTransmitMode = TransmitMode.normal;

  // ── Results from the last onDomReady ──
  /// Scripts extracted from the page (populated by onDomReady).
  List<ScriptInfo> pendingScripts = [];

  /// Execution log from the last run.
  List<String> lastLog = [];

  /// Whether any scripts are waiting for user approval.
  bool get hasPendingScripts => pendingScripts.isNotEmpty;

  /// Whether blocking outbound data.
  bool get isTransmitBlocked =>
      currentTransmitMode == TransmitMode.blocked;

  // ── DOM hook ──────────────────────────────────────────────────

  @override
  void onDomReady(Document document) {
    pendingScripts = [];
    lastLog = [];

    if (currentMode == ScriptMode.off) {
      // Remove all script elements so they leave no trace.
      _removeAllScripts(document);
      return;
    }

    // Extract script info and remove <script> tags from the DOM.
    pendingScripts = _extractScripts(document);
    _removeAllScripts(document);

    if (pendingScripts.isEmpty) return;

    PaneLogger.info(
        'script_manager: found ${pendingScripts.length} scripts '
        '(mode: ${currentMode.label})');
  }

  // ── Network hook (transmission blocking) ──────────────────────

  /// When transmit is blocked AND a page has already loaded, this flag
  /// is set by the browser shell to block subsequent outbound requests.
  bool blockSubsequentRequests = false;

  @override
  FetchRequest? onBeforeRequest(FetchRequest request) {
    if (!blockSubsequentRequests) return request;

    // Allow CSS, images, and page loads; block everything else.
    final url = request.url.toLowerCase();
    if (url.endsWith('.css') ||
        url.endsWith('.png') ||
        url.endsWith('.jpg') ||
        url.endsWith('.jpeg') ||
        url.endsWith('.gif') ||
        url.endsWith('.svg') ||
        url.endsWith('.ico') ||
        url.endsWith('.woff') ||
        url.endsWith('.woff2')) {
      return request;
    }

    // Block XHR/fetch-style requests (detected by headers).
    final xhrHeader = request.headers['X-Requested-With'] ?? '';
    if (xhrHeader.isNotEmpty) {
      PaneLogger.info(
          'script_manager: blocked outbound XHR to ${request.url}');
      return null;
    }

    return request;
  }

  // ── Script execution ──────────────────────────────────────────

  /// Execute all pending scripts (for [ScriptMode.runAll]).
  ScriptResult executeAll(Document document) {
    final engine = ScriptEngine(
      document: document,
      blockTransmit: isTransmitBlocked,
    );

    ScriptResult? lastResult;
    for (final script in pendingScripts) {
      if (script.content.isNotEmpty) {
        lastResult = engine.execute(script.content);
      }
    }

    lastLog = lastResult?.log ?? [];
    return lastResult ?? const ScriptResult();
  }

  /// Execute only approved scripts (for [ScriptMode.askEach]).
  ScriptResult executeApproved(Document document) {
    final approved = pendingScripts.where((s) => s.approved).toList();
    if (approved.isEmpty) return const ScriptResult();

    final engine = ScriptEngine(
      document: document,
      blockTransmit: isTransmitBlocked,
    );

    ScriptResult? lastResult;
    for (final script in approved) {
      if (script.content.isNotEmpty) {
        lastResult = engine.execute(script.content);
      }
    }

    lastLog = lastResult?.log ?? [];
    return lastResult ?? const ScriptResult();
  }

  // ── Internals ─────────────────────────────────────────────────

  /// Collect all <script> elements and their content.
  List<ScriptInfo> _extractScripts(Document document) {
    final scripts = <ScriptInfo>[];

    for (final node in document.descendants.toList()) {
      if (node is Element && node.tagName == 'script') {
        final src = node.attributes['src'];
        final type = node.attributes['type']?.toLowerCase() ?? '';

        // Skip non-JS script types (JSON-LD, templates, etc.).
        if (type.isNotEmpty &&
            !type.contains('javascript') &&
            !type.contains('ecmascript')) {
          continue;
        }

        // Inline script content.
        final content = node.textContent.trim();

        scripts.add(ScriptInfo(
          src: src,
          content: content,
        ));
      }
    }

    return scripts;
  }

  /// Remove all <script> elements from the document.
  void _removeAllScripts(Document document) {
    final toRemove = <Element>[];
    for (final node in document.descendants) {
      if (node is Element && node.tagName == 'script') {
        toRemove.add(node);
      }
    }
    for (final el in toRemove) {
      el.parent?.removeChild(el);
    }
  }
}
