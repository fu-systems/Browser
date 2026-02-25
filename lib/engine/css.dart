/// CSS model and parser — pure Dart, no Flutter dependency.
///
/// Parses CSS text into a Stylesheet of Rules. Each Rule has a Selector
/// and a map of property declarations. Also handles inline style parsing
/// and specificity calculation.

// ── Data model ──────────────────────────────────────────────────────

class Stylesheet {
  final List<Rule> rules;
  Stylesheet(this.rules);

  @override
  String toString() => 'Stylesheet(${rules.length} rules)';
}

class Rule {
  final Selector selector;
  final Map<String, String> declarations;
  Rule(this.selector, this.declarations);
}

/// A simplified CSS selector.
///
/// Supports: tag, .class, #id, tag.class, tag#id, and comma-separated
/// groups (each alternative is a separate Selector). Descendant combinators
/// (space-separated) are supported with [ancestors].
class Selector {
  final String? tag;
  final String? id;
  final List<String> classes;
  final Selector? ancestor; // For descendant combinator: "div p" → p has ancestor div.

  Selector({this.tag, this.id, this.classes = const [], this.ancestor});

  /// Specificity as (id-count, class-count, tag-count).
  Specificity get specificity {
    int a = id != null ? 1 : 0;
    int b = classes.length;
    int c = tag != null ? 1 : 0;
    if (ancestor != null) {
      final as_ = ancestor!.specificity;
      a += as_.a;
      b += as_.b;
      c += as_.c;
    }
    return Specificity(a, b, c);
  }

  @override
  String toString() {
    final parts = <String>[];
    if (ancestor != null) parts.add('$ancestor ');
    if (tag != null) parts.add(tag!);
    if (id != null) parts.add('#$id');
    for (final c in classes) {
      parts.add('.$c');
    }
    return parts.join();
  }
}

class Specificity implements Comparable<Specificity> {
  final int a, b, c;
  const Specificity(this.a, this.b, this.c);

  /// Inline styles use this.
  static const inline = Specificity(1, 0, 0);

  @override
  int compareTo(Specificity other) {
    if (a != other.a) return a.compareTo(other.a);
    if (b != other.b) return b.compareTo(other.b);
    return c.compareTo(other.c);
  }

  bool operator >(Specificity other) => compareTo(other) > 0;
  bool operator <(Specificity other) => compareTo(other) < 0;
  bool operator >=(Specificity other) => compareTo(other) >= 0;

  @override
  String toString() => '($a,$b,$c)';
}

// ── CSS parser ──────────────────────────────────────────────────────

class CssParser {
  final String _input;
  int _pos = 0;

  CssParser(this._input);

  static Stylesheet parse(String css) {
    return CssParser(css)._parseStylesheet();
  }

  /// Parse an inline style attribute value: "color: red; font-size: 14px"
  static Map<String, String> parseInlineStyle(String style) {
    return CssParser(style)._parseDeclarations();
  }

  // ── Character helpers ───────────────────────────────────────────

  bool get _eof => _pos >= _input.length;
  String get _current => _input[_pos];

  void _advance([int n = 1]) => _pos += n;

  void _skipWhitespace() {
    while (!_eof && _isWhitespace(_current)) _advance();
  }

  bool _isWhitespace(String c) =>
      c == ' ' || c == '\t' || c == '\n' || c == '\r';

  bool _startsWith(String s) => _input.startsWith(s, _pos);

  String _consumeWhile(bool Function(String) test) {
    final start = _pos;
    while (!_eof && test(_current)) _advance();
    return _input.substring(start, _pos);
  }

  // ── Stylesheet parsing ──────────────────────────────────────────

  Stylesheet _parseStylesheet() {
    final rules = <Rule>[];
    while (!_eof) {
      _skipWhitespace();
      if (_eof) break;

      // Skip comments.
      if (_startsWith('/*')) {
        _skipComment();
        continue;
      }

      // Skip @-rules (media queries, etc.) — not supported in Phase 1.
      if (!_eof && _current == '@') {
        _skipAtRule();
        continue;
      }

      // Parse a rule.
      final rule = _parseRule();
      if (rule != null) rules.addAll(rule);
    }
    return Stylesheet(rules);
  }

  void _skipComment() {
    _advance(2); // skip /*
    while (!_eof && !_startsWith('*/')) _advance();
    if (!_eof) _advance(2); // skip */
  }

  void _skipAtRule() {
    // Skip until { ... } block or ;
    while (!_eof) {
      if (_current == ';') {
        _advance();
        return;
      }
      if (_current == '{') {
        _skipBlock();
        return;
      }
      _advance();
    }
  }

  void _skipBlock() {
    int depth = 0;
    while (!_eof) {
      if (_current == '{') depth++;
      if (_current == '}') {
        depth--;
        if (depth == 0) {
          _advance();
          return;
        }
      }
      _advance();
    }
  }

  /// Parse one rule, which may have comma-separated selectors
  /// producing multiple Rules sharing the same declarations.
  List<Rule>? _parseRule() {
    // Parse selector text.
    final selectorText = _consumeWhile((c) => c != '{').trim();
    if (_eof || selectorText.isEmpty) return null;

    _advance(); // skip {
    final declarations = _parseDeclarations();

    // Skip closing brace.
    _skipWhitespace();
    if (!_eof && _current == '}') _advance();

    // Split comma-separated selectors.
    final selectorStrings = selectorText.split(',').map((s) => s.trim()).where((s) => s.isNotEmpty);
    final rules = <Rule>[];
    for (final selStr in selectorStrings) {
      final selector = _parseSelectorString(selStr);
      if (selector != null) {
        rules.add(Rule(selector, Map.of(declarations)));
      }
    }
    return rules.isEmpty ? null : rules;
  }

  Map<String, String> _parseDeclarations() {
    final decls = <String, String>{};
    while (!_eof && _current != '}') {
      _skipWhitespace();
      if (_eof || _current == '}') break;

      // Skip comments inside declarations.
      if (_startsWith('/*')) {
        _skipComment();
        continue;
      }

      // Property name.
      final property = _consumeWhile((c) => c != ':' && c != '}' && c != ';')
          .trim()
          .toLowerCase();

      if (_eof || _current == '}') break;
      if (_current == ';') {
        _advance();
        continue;
      }

      _advance(); // skip :
      _skipWhitespace();

      // Value — consume until ; or } but handle parentheses (e.g., rgb(...)).
      final value = _parseValue();
      if (property.isNotEmpty && value.isNotEmpty) {
        // Handle !important by stripping it (we don't prioritize it specially in Phase 1).
        final cleanValue = value
            .replaceAll(RegExp(r'\s*!important\s*$', caseSensitive: false), '')
            .trim();
        decls[property] = cleanValue;
      }
    }
    return decls;
  }

  String _parseValue() {
    final buf = StringBuffer();
    int parenDepth = 0;
    while (!_eof) {
      if (_current == '(') parenDepth++;
      if (_current == ')') parenDepth--;
      if (parenDepth == 0 && (_current == ';' || _current == '}')) break;
      buf.write(_current);
      _advance();
    }
    if (!_eof && _current == ';') _advance();
    return buf.toString().trim();
  }

  // ── Selector parsing ────────────────────────────────────────────

  /// Parse a single selector string like "div.foo #bar p".
  Selector? _parseSelectorString(String s) {
    final parts = s.trim().split(RegExp(r'\s+')).where((p) => p.isNotEmpty).toList();
    if (parts.isEmpty) return null;

    Selector? current;
    for (final part in parts) {
      current = _parseSimpleSelector(part, current);
    }
    return current;
  }

  /// Parse "div.class#id" into a Selector.
  Selector _parseSimpleSelector(String s, Selector? ancestor) {
    String? tag;
    String? id;
    final classes = <String>[];

    int i = 0;
    // Tag name (starts with a letter).
    if (i < s.length && s[i] != '.' && s[i] != '#') {
      final start = i;
      while (i < s.length && s[i] != '.' && s[i] != '#') i++;
      tag = s.substring(start, i).toLowerCase();
      if (tag == '*') tag = null; // Universal selector.
    }

    while (i < s.length) {
      if (s[i] == '#') {
        i++;
        final start = i;
        while (i < s.length && s[i] != '.' && s[i] != '#') i++;
        id = s.substring(start, i);
      } else if (s[i] == '.') {
        i++;
        final start = i;
        while (i < s.length && s[i] != '.' && s[i] != '#') i++;
        classes.add(s.substring(start, i));
      } else {
        i++;
      }
    }

    return Selector(tag: tag, id: id, classes: classes, ancestor: ancestor);
  }
}
