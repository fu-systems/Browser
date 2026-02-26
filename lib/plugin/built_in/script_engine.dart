/// Script engine — minimal JavaScript evaluator for Pane browser.
///
/// Pattern-based engine that handles the most common DOM-manipulation
/// and information-gathering patterns found in real-world scripts.
/// Unknown constructs are logged and skipped.

import '../../engine/dom.dart';

/// Result of executing one or more scripts.
class ScriptResult {
  /// Console output captured from console.log / console.warn / etc.
  final List<String> log;

  /// Alert messages captured from alert().
  final List<String> alerts;

  /// If a script set window.location, the target URL.
  final String? pendingNavigation;

  /// Whether the DOM was modified during execution.
  final bool domModified;

  /// Statements that could not be executed.
  final List<String> skipped;

  const ScriptResult({
    this.log = const [],
    this.alerts = const [],
    this.pendingNavigation,
    this.domModified = false,
    this.skipped = const [],
  });
}

/// Lightweight JavaScript evaluator that operates on the Pane DOM.
///
/// Recognises common patterns (document.write, console.log, element
/// manipulation, etc.) via regex matching and executes them directly
/// against the [Document] tree. Unknown constructs are silently
/// skipped and reported in [ScriptResult.skipped].
class ScriptEngine {
  final Document document;
  final bool blockTransmit;

  // ── Execution state ──
  final List<String> _log = [];
  final List<String> _alerts = [];
  final List<String> _skipped = [];
  String? _pendingNav;
  bool _domModified = false;
  final Map<String, String> _vars = {};

  ScriptEngine({required this.document, this.blockTransmit = false});

  /// Execute [source] against the document and return results.
  ScriptResult execute(String source) {
    final cleaned = _stripComments(source);
    final stmts = _splitStatements(cleaned);

    for (final stmt in stmts) {
      final s = stmt.trim();
      if (s.isEmpty) continue;
      _executeStatement(s);
    }

    return ScriptResult(
      log: List.unmodifiable(_log),
      alerts: List.unmodifiable(_alerts),
      pendingNavigation: _pendingNav,
      domModified: _domModified,
      skipped: List.unmodifiable(_skipped),
    );
  }

  // ── Statement dispatch ────────────────────────────────────────

  void _executeStatement(String stmt) {
    if (_tryDocumentWrite(stmt)) return;
    if (_tryDocumentTitle(stmt)) return;
    if (_tryConsoleLog(stmt)) return;
    if (_tryAlert(stmt)) return;
    if (_tryWindowLocation(stmt)) return;
    if (_tryVarDeclaration(stmt)) return;
    if (_tryElementById(stmt)) return;
    if (_tryQuerySelector(stmt)) return;
    if (_tryElementInnerHtml(stmt)) return;
    if (_tryElementTextContent(stmt)) return;
    if (_tryElementSetAttribute(stmt)) return;
    if (_tryElementStyleSet(stmt)) return;
    if (_tryElementClassChange(stmt)) return;
    if (_tryCreateElement(stmt)) return;
    if (_tryAppendChild(stmt)) return;
    if (_tryRemoveElement(stmt)) return;
    if (_tryDocumentCookie(stmt)) return;
    if (_tryFetchXhr(stmt)) return;

    _skipped.add(stmt);
  }

  // ── Pattern matchers ──────────────────────────────────────────

  // document.write / document.writeln
  static final _reDocWrite =
      RegExp(r'''document\.(?:write|writeln)\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryDocumentWrite(String s) {
    final m = _reDocWrite.firstMatch(s);
    if (m == null) return false;
    final raw = m.group(1)!;
    final value = _resolveStringArg(raw);
    if (value == null) return false;

    // Inject as text nodes into <body>.
    final body = document.body;
    if (body != null) {
      body.appendChild(Text(value));
      _domModified = true;
    }
    _log.add('[engine] document.write: ${_truncate(value, 80)}');
    return true;
  }

  // document.title = "..."
  static final _reDocTitle =
      RegExp(r'''document\.title\s*=\s*([\s\S]*?)$''');

  bool _tryDocumentTitle(String s) {
    final m = _reDocTitle.firstMatch(s);
    if (m == null) return false;
    final value = _resolveStringArg(m.group(1)!.trim());
    if (value == null) return false;

    // Find <title> element and update it.
    for (final node in document.descendants) {
      if (node is Element && node.tagName == 'title') {
        node.children.clear();
        node.appendChild(Text(value));
        _domModified = true;
        break;
      }
    }
    _log.add('[engine] document.title = "$value"');
    return true;
  }

  // console.log / console.warn / console.error / console.info
  static final _reConsole =
      RegExp(r'''console\.(log|warn|error|info)\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryConsoleLog(String s) {
    final m = _reConsole.firstMatch(s);
    if (m == null) return false;
    final level = m.group(1)!;
    final raw = m.group(2)!;
    final value = _resolveStringArg(raw) ?? raw;
    _log.add('[console.$level] $value');
    return true;
  }

  // alert("...")
  static final _reAlert = RegExp(r'''alert\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryAlert(String s) {
    final m = _reAlert.firstMatch(s);
    if (m == null) return false;
    final value = _resolveStringArg(m.group(1)!) ?? m.group(1)!;
    _alerts.add(value);
    _log.add('[engine] alert("${_truncate(value, 60)}")');
    return true;
  }

  // window.location = "..." / window.location.href = "..."
  static final _reLocation =
      RegExp(r'''window\.location(?:\.href)?\s*=\s*([\s\S]*?)$''');

  bool _tryWindowLocation(String s) {
    final m = _reLocation.firstMatch(s);
    if (m == null) return false;
    final value = _resolveStringArg(m.group(1)!.trim());
    if (value == null) return false;

    if (blockTransmit) {
      _log.add('[blocked] navigation to $value (outbound blocked)');
    } else {
      _pendingNav = value;
      _log.add('[engine] window.location = "$value"');
    }
    return true;
  }

  // var / let / const declarations
  static final _reVar =
      RegExp(r'''(?:var|let|const)\s+(\w+)\s*=\s*([\s\S]*?)$''');

  bool _tryVarDeclaration(String s) {
    final m = _reVar.firstMatch(s);
    if (m == null) return false;
    final name = m.group(1)!;
    final raw = m.group(2)!.trim();
    final value = _resolveStringArg(raw) ?? raw;
    _vars[name] = value;
    return true;
  }

  // document.getElementById("id")  — captures into a variable
  static final _reGetById = RegExp(
      r'''(?:(?:var|let|const)\s+)?(\w+)\s*=\s*document\.getElementById\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryElementById(String s) {
    final m = _reGetById.firstMatch(s);
    if (m == null) return false;
    final varName = m.group(1)!;
    final id = _resolveStringArg(m.group(2)!);
    if (id == null) return false;

    // Store the element reference as "ref:<id>".
    _vars[varName] = 'ref:$id';
    return true;
  }

  // document.querySelector("selector")
  static final _reQuerySelector = RegExp(
      r'''(?:(?:var|let|const)\s+)?(\w+)\s*=\s*document\.querySelector\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryQuerySelector(String s) {
    final m = _reQuerySelector.firstMatch(s);
    if (m == null) return false;
    final varName = m.group(1)!;
    final selector = _resolveStringArg(m.group(2)!);
    if (selector == null) return false;

    _vars[varName] = 'qs:$selector';
    return true;
  }

  // element.innerHTML = "..."
  static final _reInnerHtml =
      RegExp(r'''(\w+)\.innerHTML\s*=\s*([\s\S]*?)$''');

  bool _tryElementInnerHtml(String s) {
    final m = _reInnerHtml.firstMatch(s);
    if (m == null) return false;
    final el = _resolveElement(m.group(1)!);
    if (el == null) return false;
    final value = _resolveStringArg(m.group(2)!.trim());
    if (value == null) return false;

    el.children.clear();
    el.appendChild(Text(value));
    _domModified = true;
    _log.add('[engine] ${m.group(1)}.innerHTML = "${_truncate(value, 60)}"');
    return true;
  }

  // element.textContent = "..."
  static final _reTextContent =
      RegExp(r'''(\w+)\.textContent\s*=\s*([\s\S]*?)$''');

  bool _tryElementTextContent(String s) {
    final m = _reTextContent.firstMatch(s);
    if (m == null) return false;
    final el = _resolveElement(m.group(1)!);
    if (el == null) return false;
    final value = _resolveStringArg(m.group(2)!.trim());
    if (value == null) return false;

    el.children.clear();
    el.appendChild(Text(value));
    _domModified = true;
    _log.add(
        '[engine] ${m.group(1)}.textContent = "${_truncate(value, 60)}"');
    return true;
  }

  // element.setAttribute("name", "value")
  static final _reSetAttr = RegExp(
      r'''(\w+)\.setAttribute\s*\(\s*([\s\S]*?)\s*,\s*([\s\S]*?)\s*\)''');

  bool _tryElementSetAttribute(String s) {
    final m = _reSetAttr.firstMatch(s);
    if (m == null) return false;
    final el = _resolveElement(m.group(1)!);
    if (el == null) return false;
    final attrName = _resolveStringArg(m.group(2)!);
    final attrValue = _resolveStringArg(m.group(3)!);
    if (attrName == null) return false;

    el.attributes[attrName] = attrValue ?? '';
    _domModified = true;
    _log.add('[engine] setAttribute("$attrName", "$attrValue")');
    return true;
  }

  // element.style.property = "value"
  static final _reStyleSet =
      RegExp(r'''(\w+)\.style\.(\w+)\s*=\s*([\s\S]*?)$''');

  bool _tryElementStyleSet(String s) {
    final m = _reStyleSet.firstMatch(s);
    if (m == null) return false;
    final el = _resolveElement(m.group(1)!);
    if (el == null) return false;
    final prop = _camelToKebab(m.group(2)!);
    final value = _resolveStringArg(m.group(3)!.trim()) ?? m.group(3)!.trim();

    // Append to existing inline style.
    final existing = el.attributes['style'] ?? '';
    el.attributes['style'] =
        existing.isEmpty ? '$prop: $value' : '$existing; $prop: $value';
    _domModified = true;
    _log.add('[engine] style.$prop = "$value"');
    return true;
  }

  // element.className = "..." / element.classList.add("...")
  static final _reClassName =
      RegExp(r'''(\w+)\.className\s*=\s*([\s\S]*?)$''');
  static final _reClassListAdd =
      RegExp(r'''(\w+)\.classList\.add\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryElementClassChange(String s) {
    var m = _reClassName.firstMatch(s);
    if (m != null) {
      final el = _resolveElement(m.group(1)!);
      if (el == null) return false;
      final value = _resolveStringArg(m.group(2)!.trim()) ?? '';
      el.attributes['class'] = value;
      _domModified = true;
      return true;
    }

    m = _reClassListAdd.firstMatch(s);
    if (m != null) {
      final el = _resolveElement(m.group(1)!);
      if (el == null) return false;
      final cls = _resolveStringArg(m.group(2)!);
      if (cls == null) return false;
      final existing = el.attributes['class'] ?? '';
      el.attributes['class'] = '$existing $cls'.trim();
      _domModified = true;
      return true;
    }

    return false;
  }

  // document.createElement("tag") → variable
  static final _reCreateElement = RegExp(
      r'''(?:(?:var|let|const)\s+)?(\w+)\s*=\s*document\.createElement\s*\(\s*([\s\S]*?)\s*\)''');

  bool _tryCreateElement(String s) {
    final m = _reCreateElement.firstMatch(s);
    if (m == null) return false;
    final varName = m.group(1)!;
    final tag = _resolveStringArg(m.group(2)!);
    if (tag == null) return false;

    // Store as a virtual element reference.
    _vars[varName] = 'new:$tag';
    _log.add('[engine] createElement("$tag") → $varName');
    return true;
  }

  // parent.appendChild(child)
  static final _reAppendChild =
      RegExp(r'''(\w+)\.appendChild\s*\(\s*(\w+)\s*\)''');

  bool _tryAppendChild(String s) {
    final m = _reAppendChild.firstMatch(s);
    if (m == null) return false;
    final parent = _resolveElement(m.group(1)!);
    final childRef = _vars[m.group(2)!];
    if (parent == null || childRef == null) return false;

    if (childRef.startsWith('new:')) {
      final tag = childRef.substring(4);
      parent.appendChild(Element(tag, {}));
      _domModified = true;
      _log.add('[engine] appendChild(<$tag>)');
    }
    return true;
  }

  // element.remove() / parent.removeChild(child)
  static final _reRemove = RegExp(r'''(\w+)\.remove\s*\(\s*\)''');

  bool _tryRemoveElement(String s) {
    final m = _reRemove.firstMatch(s);
    if (m == null) return false;
    final el = _resolveElement(m.group(1)!);
    if (el == null) return false;

    el.parent?.removeChild(el);
    _domModified = true;
    _log.add('[engine] ${m.group(1)}.remove()');
    return true;
  }

  // document.cookie = "..."
  static final _reDocCookie =
      RegExp(r'''document\.cookie\s*=\s*([\s\S]*?)$''');

  bool _tryDocumentCookie(String s) {
    final m = _reDocCookie.firstMatch(s);
    if (m == null) return false;
    final value = _resolveStringArg(m.group(1)!.trim());

    if (blockTransmit) {
      _log.add('[blocked] document.cookie write (outbound blocked)');
    } else {
      _log.add('[engine] document.cookie = "${_truncate(value ?? "", 60)}"');
    }
    return true;
  }

  // fetch() / XMLHttpRequest / navigator.sendBeacon
  static final _reFetch = RegExp(
      r'''(?:fetch|new\s+XMLHttpRequest|navigator\.sendBeacon)\s*\(''');

  bool _tryFetchXhr(String s) {
    final m = _reFetch.firstMatch(s);
    if (m == null) return false;

    if (blockTransmit) {
      _log.add('[blocked] network request (outbound blocked)');
    } else {
      _log.add('[engine] network request: ${_truncate(s, 80)}');
    }
    return true;
  }

  // ── Helper methods ────────────────────────────────────────────

  /// Resolve a variable name to a DOM element.
  Element? _resolveElement(String name) {
    if (name == 'document') return document.documentElement;

    final ref = _vars[name];
    if (ref == null) return null;

    if (ref.startsWith('ref:')) {
      final id = ref.substring(4);
      return _findById(id);
    }
    if (ref.startsWith('qs:')) {
      final sel = ref.substring(3);
      return _querySelector(sel);
    }
    // If it's "document.body" stored directly.
    if (name == 'body') return document.body;

    return null;
  }

  Element? _findById(String id) {
    for (final node in document.descendants) {
      if (node is Element && node.attributes['id'] == id) return node;
    }
    return null;
  }

  Element? _querySelector(String selector) {
    // Simple tag-name matching only.
    final tag = selector.trim().toLowerCase();
    for (final node in document.descendants) {
      if (node is Element && node.tagName == tag) return node;
    }
    return null;
  }

  /// Resolve a string argument: strip quotes, handle concatenation,
  /// resolve variable references.
  String? _resolveStringArg(String raw) {
    final trimmed = raw.trim();
    if (trimmed.isEmpty) return null;

    // Handle string concatenation with +.
    if (trimmed.contains('+')) {
      final parts = _splitConcatenation(trimmed);
      final resolved = parts.map(_resolveSingleValue).toList();
      if (resolved.any((r) => r == null)) return null;
      return resolved.join();
    }

    return _resolveSingleValue(trimmed);
  }

  String? _resolveSingleValue(String raw) {
    final s = raw.trim();

    // Double-quoted string.
    if (s.startsWith('"') && s.endsWith('"') && s.length >= 2) {
      return _unescapeString(s.substring(1, s.length - 1));
    }
    // Single-quoted string.
    if (s.startsWith("'") && s.endsWith("'") && s.length >= 2) {
      return _unescapeString(s.substring(1, s.length - 1));
    }
    // Backtick template literal (no interpolation).
    if (s.startsWith('`') && s.endsWith('`') && s.length >= 2) {
      return s.substring(1, s.length - 1);
    }
    // Numeric literal.
    if (RegExp(r'^-?\d+\.?\d*$').hasMatch(s)) return s;
    // Boolean / null / undefined.
    if (s == 'true' || s == 'false' || s == 'null' || s == 'undefined') {
      return s;
    }
    // Variable reference.
    if (_vars.containsKey(s)) return _vars[s];

    return null;
  }

  /// Split a string-concatenation expression on top-level `+` operators.
  List<String> _splitConcatenation(String expr) {
    final parts = <String>[];
    final buf = StringBuffer();
    int depth = 0;
    String? quote;

    for (int i = 0; i < expr.length; i++) {
      final ch = expr[i];

      if (quote != null) {
        buf.write(ch);
        if (ch == quote && (i == 0 || expr[i - 1] != '\\')) quote = null;
        continue;
      }

      if (ch == '"' || ch == "'" || ch == '`') {
        quote = ch;
        buf.write(ch);
        continue;
      }

      if (ch == '(' || ch == '[') {
        depth++;
        buf.write(ch);
        continue;
      }
      if (ch == ')' || ch == ']') {
        depth--;
        buf.write(ch);
        continue;
      }

      if (ch == '+' && depth == 0) {
        parts.add(buf.toString());
        buf.clear();
        continue;
      }

      buf.write(ch);
    }

    if (buf.isNotEmpty) parts.add(buf.toString());
    return parts;
  }

  String _unescapeString(String s) {
    return s
        .replaceAll(r'\n', '\n')
        .replaceAll(r'\t', '\t')
        .replaceAll(r'\"', '"')
        .replaceAll(r"\'", "'")
        .replaceAll(r'\\', '\\');
  }

  /// Strip single-line and multi-line comments.
  String _stripComments(String source) {
    final buf = StringBuffer();
    int i = 0;
    String? quote;

    while (i < source.length) {
      final ch = source[i];

      // Inside a string literal — pass through.
      if (quote != null) {
        buf.write(ch);
        if (ch == quote && (i == 0 || source[i - 1] != '\\')) quote = null;
        i++;
        continue;
      }

      // Start of string literal.
      if (ch == '"' || ch == "'" || ch == '`') {
        quote = ch;
        buf.write(ch);
        i++;
        continue;
      }

      // Single-line comment.
      if (i + 1 < source.length && ch == '/' && source[i + 1] == '/') {
        while (i < source.length && source[i] != '\n') {
          i++;
        }
        continue;
      }

      // Multi-line comment.
      if (i + 1 < source.length && ch == '/' && source[i + 1] == '*') {
        i += 2;
        while (i + 1 < source.length &&
            !(source[i] == '*' && source[i + 1] == '/')) {
          i++;
        }
        i += 2; // skip */
        continue;
      }

      buf.write(ch);
      i++;
    }

    return buf.toString();
  }

  /// Split source into statements, respecting strings and braces.
  List<String> _splitStatements(String source) {
    final stmts = <String>[];
    final buf = StringBuffer();
    int braceDepth = 0;
    String? quote;

    for (int i = 0; i < source.length; i++) {
      final ch = source[i];

      if (quote != null) {
        buf.write(ch);
        if (ch == quote && (i == 0 || source[i - 1] != '\\')) quote = null;
        continue;
      }

      if (ch == '"' || ch == "'" || ch == '`') {
        quote = ch;
        buf.write(ch);
        continue;
      }

      if (ch == '{') {
        braceDepth++;
        buf.write(ch);
        continue;
      }
      if (ch == '}') {
        braceDepth--;
        buf.write(ch);
        if (braceDepth <= 0) {
          stmts.add(buf.toString());
          buf.clear();
          braceDepth = 0;
        }
        continue;
      }

      if (ch == ';' && braceDepth == 0) {
        stmts.add(buf.toString());
        buf.clear();
        continue;
      }

      buf.write(ch);
    }

    if (buf.isNotEmpty) stmts.add(buf.toString());
    return stmts;
  }

  /// Convert camelCase to kebab-case (e.g., backgroundColor → background-color).
  String _camelToKebab(String s) {
    return s.replaceAllMapped(
        RegExp(r'[A-Z]'), (m) => '-${m.group(0)!.toLowerCase()}');
  }

  String _truncate(String s, int max) =>
      s.length <= max ? s : '${s.substring(0, max)}...';
}
