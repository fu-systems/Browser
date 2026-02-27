/// CSS model and parser — pure Dart, no Flutter dependency.
///
/// Implements CSS Selectors Level 4, @media queries, @import,
/// !important, and the full CSS value syntax including calc(),
/// var(), and modern units (rem, vh, vw, vmin, vmax).
///
/// Reference: W3C CSS Syntax Module Level 3
///            W3C Selectors Level 4
///            W3C CSS Cascade Level 4

// ── Data model ──────────────────────────────────────────────────────

class Stylesheet {
  final List<Rule> rules;
  final List<String> imports; // @import URLs
  Stylesheet(this.rules, [this.imports = const []]);

  @override
  String toString() => 'Stylesheet(${rules.length} rules, ${imports.length} imports)';
}

class Rule {
  final Selector selector;
  final Map<String, CssValue> declarations;
  Rule(this.selector, this.declarations);
}

/// A CSS property value that tracks !important.
class CssValue {
  final String value;
  final bool important;
  const CssValue(this.value, [this.important = false]);

  @override
  String toString() => important ? '$value !important' : value;
}

// ── Selector model (CSS Selectors Level 4) ──────────────────────────

/// A complete CSS selector, which is a chain of compound selectors
/// separated by combinators. Evaluated right-to-left.
class Selector {
  /// The compound selectors in this chain, from rightmost (subject) to leftmost.
  final List<CompoundSelector> compounds;
  /// The combinators between compounds (one fewer than compounds).
  /// combinators[0] is between compounds[0] (subject) and compounds[1].
  final List<Combinator> combinators;

  Selector(this.compounds, this.combinators);

  /// Specificity as (id-count, class-count, tag-count).
  Specificity get specificity {
    int a = 0, b = 0, c = 0;
    for (final compound in compounds) {
      final s = compound.specificity;
      a += s.a;
      b += s.b;
      c += s.c;
    }
    return Specificity(a, b, c);
  }

  @override
  String toString() {
    if (compounds.isEmpty) return '';
    final buf = StringBuffer();
    // Print left-to-right (compounds are stored right-to-left).
    for (int i = compounds.length - 1; i >= 0; i--) {
      buf.write(compounds[i]);
      if (i > 0) {
        switch (combinators[i - 1]) {
          case Combinator.descendant:
            buf.write(' ');
          case Combinator.child:
            buf.write(' > ');
          case Combinator.adjacentSibling:
            buf.write(' + ');
          case Combinator.generalSibling:
            buf.write(' ~ ');
        }
      }
    }
    return buf.toString();
  }
}

enum Combinator { descendant, child, adjacentSibling, generalSibling }

/// A compound selector: a sequence of simple selectors with no combinator.
/// e.g., "div.foo#bar[type=text]:hover"
class CompoundSelector {
  String? tag;          // null = universal (*)
  String? id;
  List<String> classes;
  List<AttributeSelector> attributes;
  List<PseudoSelector> pseudoClasses;
  PseudoElement? pseudoElement;

  CompoundSelector({
    this.tag,
    this.id,
    this.classes = const [],
    this.attributes = const [],
    this.pseudoClasses = const [],
    this.pseudoElement,
  });

  Specificity get specificity {
    int a = id != null ? 1 : 0;
    int b = classes.length + attributes.length + pseudoClasses.length;
    int c = (tag != null ? 1 : 0) + (pseudoElement != null ? 1 : 0);
    // :not() adds its argument's specificity, not the pseudo-class itself.
    for (final pseudo in pseudoClasses) {
      if (pseudo.name == 'not' && pseudo.selectorArg != null) {
        final inner = pseudo.selectorArg!.specificity;
        a += inner.a;
        b += inner.b - 1; // We already counted :not itself, subtract it.
        c += inner.c;
      }
      if (pseudo.name == 'is' || pseudo.name == 'matches' || pseudo.name == 'where') {
        if (pseudo.selectorArg != null) {
          final inner = pseudo.selectorArg!.specificity;
          if (pseudo.name == 'where') {
            b -= 1; // :where() has zero specificity
          } else {
            a += inner.a;
            b += inner.b - 1;
            c += inner.c;
          }
        }
      }
    }
    return Specificity(a, b, c);
  }

  @override
  String toString() {
    final buf = StringBuffer();
    if (tag != null) buf.write(tag);
    if (id != null) buf.write('#$id');
    for (final c in classes) buf.write('.$c');
    for (final a in attributes) buf.write(a);
    for (final p in pseudoClasses) buf.write(p);
    if (pseudoElement != null) buf.write(pseudoElement);
    return buf.toString();
  }
}

/// CSS attribute selector: [attr], [attr=val], [attr~=val], etc.
class AttributeSelector {
  final String name;
  final String? op;     // null, =, ~=, |=, ^=, $=, *=
  final String? value;
  final bool caseInsensitive; // [attr=val i]

  AttributeSelector(this.name, {this.op, this.value, this.caseInsensitive = false});

  @override
  String toString() {
    if (op == null) return '[$name]';
    return '[$name$op"$value"${caseInsensitive ? ' i' : ''}]';
  }
}

/// CSS pseudo-class selector: :hover, :first-child, :nth-child(2n+1), :not(.foo)
class PseudoSelector {
  final String name;
  final String? argument;          // For functional pseudos like :nth-child(2n+1)
  final Selector? selectorArg;     // For :not(selector), :is(selector), etc.

  PseudoSelector(this.name, {this.argument, this.selectorArg});

  @override
  String toString() {
    if (selectorArg != null) return ':$name($selectorArg)';
    if (argument != null) return ':$name($argument)';
    return ':$name';
  }
}

/// CSS pseudo-element: ::before, ::after, ::first-line, ::first-letter
class PseudoElement {
  final String name;
  PseudoElement(this.name);

  @override
  String toString() => '::$name';
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

// ── Media query model ───────────────────────────────────────────────

class MediaQuery {
  final String? type;               // screen, print, all
  final bool negated;
  final List<MediaFeature> features;

  MediaQuery({this.type, this.negated = false, this.features = const []});

  /// Evaluate against viewport dimensions.
  bool evaluate(double viewportWidth, double viewportHeight) {
    // Type check.
    bool result = true;
    if (type != null && type != 'all') {
      final matchesType = type == 'screen'; // We're always "screen".
      if (!matchesType) result = false;
    }
    // Feature checks — all must pass.
    if (result) {
      for (final feature in features) {
        if (!feature.evaluate(viewportWidth, viewportHeight)) {
          result = false;
          break;
        }
      }
    }
    // Apply negation at the end.
    return negated ? !result : result;
  }
}

class MediaFeature {
  final String name;
  final String? value;

  MediaFeature(this.name, [this.value]);

  bool evaluate(double vw, double vh) {
    final px = value != null ? _parseMediaPx(value!) : 0;
    switch (name) {
      case 'min-width':
        return vw >= px;
      case 'max-width':
        return vw <= px;
      case 'width':
        return vw == px;
      case 'min-height':
        return vh >= px;
      case 'max-height':
        return vh <= px;
      case 'height':
        return vh == px;
      case 'orientation':
        return value == (vw > vh ? 'landscape' : 'portrait');
      case 'prefers-color-scheme':
        return value == 'light'; // We default to light.
      case 'prefers-reduced-motion':
        return value == 'no-preference';
      case 'color':
        return true; // We support color.
      case 'hover':
        return value == 'hover'; // We support hover.
      case 'pointer':
        return value == 'fine'; // Mouse pointer.
      default:
        return true; // Unknown features pass.
    }
  }
}

double _parseMediaPx(String value) {
  if (value.endsWith('px')) {
    return double.tryParse(value.replaceAll('px', '')) ?? 0;
  }
  if (value.endsWith('em') || value.endsWith('rem')) {
    final n = double.tryParse(value.replaceAll(RegExp(r'r?em'), ''));
    if (n != null) return n * 16;
  }
  return double.tryParse(value) ?? 0;
}

// ── CSS parser ──────────────────────────────────────────────────────

class CssParser {
  final String _input;
  int _pos = 0;
  final double _viewportWidth;
  final double _viewportHeight;

  CssParser(this._input, [this._viewportWidth = 1024, this._viewportHeight = 768]);

  static Stylesheet parse(String css, [double viewportWidth = 1024, double viewportHeight = 768]) {
    return CssParser(css, viewportWidth, viewportHeight)._parseStylesheet();
  }

  /// Parse an inline style attribute value: "color: red; font-size: 14px"
  static Map<String, CssValue> parseInlineStyle(String style) {
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
    final imports = <String>[];
    while (!_eof) {
      _skipWhitespace();
      if (_eof) break;

      // Skip comments.
      if (_startsWith('/*')) {
        _skipComment();
        continue;
      }

      // Handle @-rules.
      if (!_eof && _current == '@') {
        _parseAtRule(rules, imports);
        continue;
      }

      // Parse a rule.
      final rule = _parseRule();
      if (rule != null) rules.addAll(rule);
    }
    return Stylesheet(rules, imports);
  }

  void _skipComment() {
    _advance(2); // skip /*
    while (!_eof && !_startsWith('*/')) _advance();
    if (!_eof) _advance(2); // skip */
  }

  void _parseAtRule(List<Rule> rules, List<String> imports) {
    _advance(); // skip @
    final keyword = _consumeWhile((c) => c != ' ' && c != '\t' && c != '\n' && c != '{' && c != ';' && c != '(').toLowerCase();
    _skipWhitespace();

    switch (keyword) {
      case 'import':
        _parseImport(imports);
      case 'media':
        _parseMediaRule(rules);
      case 'supports':
        _parseSupportsRule(rules);
      case 'keyframes' || '-webkit-keyframes' || '-moz-keyframes':
        _skipBlock(); // Skip animations for now.
      case 'font-face':
        _skipBlock(); // Skip @font-face for now (no custom fonts without plugin).
      case 'charset':
        _skipUntilSemicolon();
      case 'namespace':
        _skipUntilSemicolon();
      case 'page':
        _skipBlock();
      case 'layer':
        _parseLayerRule(rules);
      default:
        _skipAtRuleBody();
    }
  }

  void _parseImport(List<String> imports) {
    _skipWhitespace();
    String url = '';
    if (!_eof && _current == '"') {
      url = _parseQuotedString();
    } else if (!_eof && _current == "'") {
      url = _parseQuotedString();
    } else if (_startsWith('url(')) {
      url = _parseUrlFunction();
    }
    if (url.isNotEmpty) imports.add(url);
    _skipUntilSemicolon();
  }

  String _parseQuotedString() {
    final quote = _current;
    _advance();
    final buf = StringBuffer();
    while (!_eof && _current != quote) {
      if (_current == '\\' && _pos + 1 < _input.length) {
        _advance();
        buf.write(_current);
      } else {
        buf.write(_current);
      }
      _advance();
    }
    if (!_eof) _advance(); // Skip closing quote.
    return buf.toString();
  }

  String _parseUrlFunction() {
    _advance(4); // skip url(
    _skipWhitespace();
    String url;
    if (!_eof && (_current == '"' || _current == "'")) {
      url = _parseQuotedString();
    } else {
      url = _consumeWhile((c) => c != ')').trim();
    }
    _skipWhitespace();
    if (!_eof && _current == ')') _advance();
    return url;
  }

  void _parseMediaRule(List<Rule> rules) {
    // Parse the media query condition.
    final conditionText = _consumeWhile((c) => c != '{').trim();
    if (_eof) return;
    _advance(); // skip {

    // Parse the media query.
    final queries = _parseMediaQueryList(conditionText);

    // Check if any query matches the viewport.
    final matches = queries.any((q) => q.evaluate(_viewportWidth, _viewportHeight));

    if (matches) {
      // Parse inner rules.
      while (!_eof && _current != '}') {
        _skipWhitespace();
        if (_eof || _current == '}') break;

        if (_startsWith('/*')) {
          _skipComment();
          continue;
        }

        if (_current == '@') {
          _parseAtRule(rules, []);
          continue;
        }

        final rule = _parseRule();
        if (rule != null) rules.addAll(rule);
      }
    } else {
      _skipBlockContent();
    }
    if (!_eof && _current == '}') _advance();
  }

  void _parseSupportsRule(List<Rule> rules) {
    // Parse the condition.
    final conditionText = _consumeWhile((c) => c != '{').trim();
    if (_eof) return;
    _advance(); // skip {

    // For now, assume @supports conditions are met (most CSS features).
    // A proper implementation would evaluate the condition.
    while (!_eof && _current != '}') {
      _skipWhitespace();
      if (_eof || _current == '}') break;

      if (_startsWith('/*')) {
        _skipComment();
        continue;
      }

      if (_current == '@') {
        _parseAtRule(rules, []);
        continue;
      }

      final rule = _parseRule();
      if (rule != null) rules.addAll(rule);
    }
    if (!_eof && _current == '}') _advance();
  }

  void _parseLayerRule(List<Rule> rules) {
    _skipWhitespace();
    if (!_eof && _current == '{') {
      _advance(); // skip {
      // Parse inner rules.
      while (!_eof && _current != '}') {
        _skipWhitespace();
        if (_eof || _current == '}') break;

        if (_startsWith('/*')) {
          _skipComment();
          continue;
        }

        if (_current == '@') {
          _parseAtRule(rules, []);
          continue;
        }

        final rule = _parseRule();
        if (rule != null) rules.addAll(rule);
      }
      if (!_eof && _current == '}') _advance();
    } else {
      _skipUntilSemicolon();
    }
  }

  List<MediaQuery> _parseMediaQueryList(String text) {
    return text.split(',').map((q) => _parseSingleMediaQuery(q.trim())).toList();
  }

  MediaQuery _parseSingleMediaQuery(String text) {
    if (text.isEmpty) return MediaQuery(type: 'all');

    bool negated = false;
    String? type;
    final features = <MediaFeature>[];

    // Tokenize by "and" keyword.
    final parts = text.split(RegExp(r'\band\b', caseSensitive: false)).map((p) => p.trim()).toList();

    for (int i = 0; i < parts.length; i++) {
      var part = parts[i];
      if (part.isEmpty) continue;

      if (i == 0) {
        // First part might be "not screen" or "screen" or "(min-width: 768px)".
        if (part.startsWith('not ') || part.startsWith('NOT ')) {
          negated = true;
          part = part.substring(4).trim();
        } else if (part.startsWith('only ') || part.startsWith('ONLY ')) {
          part = part.substring(5).trim();
        }

        if (!part.startsWith('(')) {
          type = part.toLowerCase();
          continue;
        }
      }

      // Parse feature: (feature-name: value) or (feature-name).
      final featureMatch = RegExp(r'\(\s*([\w-]+)\s*(?::\s*([^)]+))?\s*\)').firstMatch(part);
      if (featureMatch != null) {
        features.add(MediaFeature(
          featureMatch.group(1)!.toLowerCase(),
          featureMatch.group(2)?.trim(),
        ));
      }
    }

    return MediaQuery(type: type ?? (features.isNotEmpty ? null : 'all'), negated: negated, features: features);
  }

  void _skipBlockContent() {
    int depth = 0;
    while (!_eof) {
      if (_current == '{') depth++;
      if (_current == '}') {
        if (depth == 0) return;
        depth--;
      }
      _advance();
    }
  }

  void _skipBlock() {
    _skipWhitespace();
    if (!_eof && _current != '{') {
      _consumeWhile((c) => c != '{');
    }
    if (_eof) return;
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

  void _skipUntilSemicolon() {
    while (!_eof && _current != ';') _advance();
    if (!_eof) _advance();
  }

  void _skipAtRuleBody() {
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
    final selectors = _parseSelectorList(selectorText);
    final rules = <Rule>[];
    for (final selector in selectors) {
      rules.add(Rule(selector, Map.of(declarations)));
    }
    return rules.isEmpty ? null : rules;
  }

  Map<String, CssValue> _parseDeclarations() {
    final decls = <String, CssValue>{};
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

      // Value — consume until ; or } but handle parentheses, quotes.
      final value = _parsePropertyValue();
      if (property.isNotEmpty && value.value.isNotEmpty) {
        decls[property] = value;
      }
    }
    return decls;
  }

  CssValue _parsePropertyValue() {
    final buf = StringBuffer();
    int parenDepth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    while (!_eof) {
      final c = _current;

      // Handle quotes.
      if (c == "'" && !inDoubleQuote) {
        inSingleQuote = !inSingleQuote;
      } else if (c == '"' && !inSingleQuote) {
        inDoubleQuote = !inDoubleQuote;
      }

      if (!inSingleQuote && !inDoubleQuote) {
        // Skip inline comments inside property values.
        if (_startsWith('/*')) {
          _skipComment();
          continue;
        }
        if (c == '(') parenDepth++;
        if (c == ')') parenDepth--;
        if (parenDepth == 0 && (c == ';' || c == '}')) break;
      }

      buf.write(c);
      _advance();
    }
    if (!_eof && _current == ';') _advance();

    final raw = buf.toString().trim();
    // Check for !important.
    final importantMatch = RegExp(r'\s*!important\s*$', caseSensitive: false).firstMatch(raw);
    if (importantMatch != null) {
      return CssValue(raw.substring(0, importantMatch.start).trim(), true);
    }
    return CssValue(raw);
  }

  // ── Selector parsing (CSS Selectors Level 4) ─────────────────────

  /// Parse a comma-separated list of selectors.
  List<Selector> _parseSelectorList(String text) {
    final selectors = <Selector>[];
    // Split on commas, but not inside parentheses or brackets.
    final parts = _splitSelectorList(text);
    for (final part in parts) {
      final trimmed = part.trim();
      if (trimmed.isEmpty) continue;
      final sel = _parseSingleSelector(trimmed);
      if (sel != null) selectors.add(sel);
    }
    return selectors;
  }

  /// Split selector list on commas, respecting parentheses and brackets.
  List<String> _splitSelectorList(String text) {
    final parts = <String>[];
    final buf = StringBuffer();
    int parenDepth = 0;
    int bracketDepth = 0;
    for (int i = 0; i < text.length; i++) {
      final c = text[i];
      if (c == '(') parenDepth++;
      if (c == ')') parenDepth--;
      if (c == '[') bracketDepth++;
      if (c == ']') bracketDepth--;
      if (c == ',' && parenDepth == 0 && bracketDepth == 0) {
        parts.add(buf.toString());
        buf.clear();
      } else {
        buf.write(c);
      }
    }
    if (buf.isNotEmpty) parts.add(buf.toString());
    return parts;
  }

  /// Parse a single selector like "div > p.foo + span:hover [type=text]".
  Selector? _parseSingleSelector(String text) {
    final tokens = _tokenizeSelector(text.trim());
    if (tokens.isEmpty) return null;

    final compounds = <CompoundSelector>[];
    final combinators = <Combinator>[];

    for (final token in tokens) {
      if (token.isCombinator) {
        combinators.add(token.combinator!);
      } else {
        compounds.add(token.compound!);
      }
    }

    if (compounds.isEmpty) return null;

    // Reverse to right-to-left order (subject first).
    return Selector(compounds.reversed.toList(), combinators.reversed.toList());
  }

  /// Tokenize a selector string into compound selectors and combinators.
  List<_SelectorToken> _tokenizeSelector(String text) {
    final tokens = <_SelectorToken>[];
    int i = 0;

    while (i < text.length) {
      // Skip whitespace.
      while (i < text.length && _isWhitespace(text[i])) i++;
      if (i >= text.length) break;

      // Check for combinator.
      if (text[i] == '>') {
        tokens.add(_SelectorToken.comb(Combinator.child));
        i++;
        continue;
      }
      if (text[i] == '+') {
        tokens.add(_SelectorToken.comb(Combinator.adjacentSibling));
        i++;
        continue;
      }
      if (text[i] == '~') {
        tokens.add(_SelectorToken.comb(Combinator.generalSibling));
        i++;
        continue;
      }

      // Parse compound selector.
      final start = i;
      final compound = _parseCompoundSelectorAt(text, i);
      i = compound.$2;

      if (compound.$1 != null) {
        // If there was whitespace between the last compound and this one,
        // and no explicit combinator, it's a descendant combinator.
        if (tokens.isNotEmpty && !tokens.last.isCombinator) {
          tokens.add(_SelectorToken.comb(Combinator.descendant));
        }
        tokens.add(_SelectorToken.comp(compound.$1!));
      }

      // Safety: prevent infinite loop.
      if (i == start) i++;
    }

    return tokens;
  }

  /// Parse a compound selector starting at position i in text.
  /// Returns the compound selector and the new position.
  (CompoundSelector?, int) _parseCompoundSelectorAt(String text, int i) {
    String? tag;
    String? id;
    final classes = <String>[];
    final attributes = <AttributeSelector>[];
    final pseudoClasses = <PseudoSelector>[];
    PseudoElement? pseudoElement;

    bool parsedAnything = false;

    // Tag name or *.
    if (i < text.length && text[i] != '.' && text[i] != '#' && text[i] != '[' && text[i] != ':' &&
        !_isWhitespace(text[i]) && text[i] != '>' && text[i] != '+' && text[i] != '~') {
      final start = i;
      while (i < text.length && text[i] != '.' && text[i] != '#' && text[i] != '[' && text[i] != ':' &&
          !_isWhitespace(text[i]) && text[i] != '>' && text[i] != '+' && text[i] != '~' && text[i] != ',') {
        i++;
      }
      final name = text.substring(start, i).toLowerCase();
      tag = name == '*' ? null : name;
      parsedAnything = true;
    }

    while (i < text.length) {
      if (text[i] == '#') {
        // ID selector.
        i++;
        final start = i;
        while (i < text.length && _isSelectorChar(text[i])) i++;
        id = text.substring(start, i);
        parsedAnything = true;
      } else if (text[i] == '.') {
        // Class selector.
        i++;
        final start = i;
        while (i < text.length && _isSelectorChar(text[i])) i++;
        if (i > start) classes.add(text.substring(start, i));
        parsedAnything = true;
      } else if (text[i] == '[') {
        // Attribute selector.
        final result = _parseAttributeSelectorAt(text, i);
        if (result.$1 != null) attributes.add(result.$1!);
        i = result.$2;
        parsedAnything = true;
      } else if (text[i] == ':') {
        // Pseudo-class or pseudo-element.
        if (i + 1 < text.length && text[i + 1] == ':') {
          // Pseudo-element.
          i += 2;
          final start = i;
          while (i < text.length && _isSelectorChar(text[i])) i++;
          pseudoElement = PseudoElement(text.substring(start, i).toLowerCase());
          parsedAnything = true;
        } else {
          // Pseudo-class.
          i++;
          final start = i;
          while (i < text.length && _isSelectorChar(text[i]) && text[i] != '(') i++;
          final name = text.substring(start, i).toLowerCase();

          if (i < text.length && text[i] == '(') {
            // Functional pseudo-class.
            i++; // skip (
            int depth = 1;
            final argStart = i;
            while (i < text.length && depth > 0) {
              if (text[i] == '(') depth++;
              if (text[i] == ')') depth--;
              if (depth > 0) i++;
            }
            final arg = text.substring(argStart, i).trim();
            if (i < text.length) i++; // skip )

            if (name == 'not' || name == 'is' || name == 'matches' || name == 'where' || name == 'has') {
              // These take a selector as argument.
              final innerSelectors = _parseSelectorList(arg);
              if (innerSelectors.isNotEmpty) {
                pseudoClasses.add(PseudoSelector(name, selectorArg: innerSelectors.first));
              }
            } else {
              pseudoClasses.add(PseudoSelector(name, argument: arg));
            }
          } else {
            pseudoClasses.add(PseudoSelector(name));
          }
          parsedAnything = true;
        }
      } else {
        break;
      }
    }

    if (!parsedAnything) return (null, i);

    return (CompoundSelector(
      tag: tag,
      id: id,
      classes: classes,
      attributes: attributes,
      pseudoClasses: pseudoClasses,
      pseudoElement: pseudoElement,
    ), i);
  }

  /// Parse an attribute selector starting at [.
  (AttributeSelector?, int) _parseAttributeSelectorAt(String text, int i) {
    i++; // skip [
    // Skip whitespace.
    while (i < text.length && _isWhitespace(text[i])) i++;

    // Attribute name.
    final nameStart = i;
    while (i < text.length && text[i] != ']' && text[i] != '=' && text[i] != '~' &&
        text[i] != '|' && text[i] != '^' && text[i] != '\$' && text[i] != '*' && !_isWhitespace(text[i])) {
      i++;
    }
    final name = text.substring(nameStart, i).trim().toLowerCase();

    while (i < text.length && _isWhitespace(text[i])) i++;

    if (i >= text.length || text[i] == ']') {
      if (i < text.length) i++; // skip ]
      return (AttributeSelector(name), i);
    }

    // Operator.
    String op = '';
    if (text[i] == '=') {
      op = '=';
      i++;
    } else if (i + 1 < text.length && text[i + 1] == '=') {
      op = text[i] + '=';
      i += 2;
    } else {
      // Skip to ].
      while (i < text.length && text[i] != ']') i++;
      if (i < text.length) i++;
      return (AttributeSelector(name), i);
    }

    while (i < text.length && _isWhitespace(text[i])) i++;

    // Value.
    String value;
    if (i < text.length && (text[i] == '"' || text[i] == "'")) {
      final quote = text[i];
      i++;
      final valueStart = i;
      while (i < text.length && text[i] != quote) {
        if (text[i] == '\\' && i + 1 < text.length) i++;
        i++;
      }
      value = text.substring(valueStart, i);
      if (i < text.length) i++; // skip closing quote
    } else {
      final valueStart = i;
      while (i < text.length && text[i] != ']' && !_isWhitespace(text[i])) i++;
      value = text.substring(valueStart, i);
    }

    while (i < text.length && _isWhitespace(text[i])) i++;

    // Check for case-insensitive flag.
    bool caseInsensitive = false;
    if (i < text.length && (text[i] == 'i' || text[i] == 'I')) {
      caseInsensitive = true;
      i++;
      while (i < text.length && _isWhitespace(text[i])) i++;
    }

    if (i < text.length && text[i] == ']') i++;

    return (AttributeSelector(name, op: op, value: value, caseInsensitive: caseInsensitive), i);
  }

  bool _isSelectorChar(String c) {
    final code = c.codeUnitAt(0);
    return (code >= 0x61 && code <= 0x7A) || // a-z
        (code >= 0x41 && code <= 0x5A) || // A-Z
        (code >= 0x30 && code <= 0x39) || // 0-9
        c == '-' || c == '_' || code > 0x7F; // Includes unicode
  }
}

class _SelectorToken {
  final CompoundSelector? compound;
  final Combinator? combinator;
  bool get isCombinator => combinator != null;

  _SelectorToken.comp(CompoundSelector this.compound) : combinator = null;
  _SelectorToken.comb(Combinator this.combinator) : compound = null;
}
