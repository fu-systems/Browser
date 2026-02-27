/// HTML parser — pure Dart, no Flutter dependency.
///
/// Parses an HTML string into a DOM tree. Handles malformed markup
/// gracefully by auto-closing tags and recovering from errors.

import 'dom.dart';

class HtmlParser {
  final String _input;
  int _pos = 0;

  HtmlParser(this._input);

  /// Parse the full HTML string and return a Document.
  static Document parse(String html) {
    return HtmlParser(html)._parseDocument();
  }

  // ── Character-level helpers ──────────────────────────────────────

  bool get _eof => _pos >= _input.length;

  String get _current => _input[_pos];

  String _peek(int offset) {
    final i = _pos + offset;
    return i < _input.length ? _input[i] : '';
  }

  void _advance([int n = 1]) => _pos += n;

  void _skipWhitespace() {
    while (!_eof && _isWhitespace(_current)) {
      _advance();
    }
  }

  bool _isWhitespace(String ch) =>
      ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';

  bool _startsWith(String s) => _input.startsWith(s, _pos);

  /// Consume characters while [test] is true.
  String _consumeWhile(bool Function(String) test) {
    final start = _pos;
    while (!_eof && test(_current)) {
      _advance();
    }
    return _input.substring(start, _pos);
  }

  // ── Document parsing ─────────────────────────────────────────────

  Document _parseDocument() {
    final doc = Document();

    // Skip doctype if present.
    _skipWhitespace();
    if (_startsWith('<!') || _startsWith('<?')) {
      _skipDoctype();
    }

    // Parse nodes into the document.
    final nodes = _parseNodes(null);

    // Build proper structure: ensure <html>, <head>, <body>.
    Element? htmlEl;
    Element? headEl;
    Element? bodyEl;

    for (final node in nodes) {
      if (node is Element && node.tagName == 'html') {
        htmlEl = node;
        break;
      }
    }

    if (htmlEl == null) {
      // No <html> tag — wrap everything in one.
      htmlEl = Element('html');
      bodyEl = Element('body');
      for (final node in nodes) {
        if (node is Element && node.tagName == 'head') {
          headEl = node;
        } else if (node is Element && node.tagName == 'body') {
          bodyEl = node;
        } else {
          bodyEl!.appendChild(node);
        }
      }
      headEl ??= Element('head');
      htmlEl.appendChild(headEl);
      htmlEl.appendChild(bodyEl!);
    } else {
      // We have <html>. Ensure head and body exist.
      for (final child in htmlEl.children) {
        if (child is Element && child.tagName == 'head') headEl = child;
        if (child is Element && child.tagName == 'body') bodyEl = child;
      }
      if (headEl == null) {
        headEl = Element('head');
        htmlEl.children.insert(0, headEl);
        headEl.parent = htmlEl;
      }
      if (bodyEl == null) {
        bodyEl = Element('body');
        // Move any non-head children into body.
        final toMove = htmlEl.children
            .where((c) => c != headEl)
            .toList();
        for (final c in toMove) {
          htmlEl.removeChild(c);
          bodyEl.appendChild(c);
        }
        htmlEl.appendChild(bodyEl);
      }
    }

    doc.appendChild(htmlEl);
    return doc;
  }

  void _skipDoctype() {
    // Skip anything starting with <! until >
    while (!_eof && _current != '>') {
      _advance();
    }
    if (!_eof) _advance(); // skip >
  }

  // ── Node parsing ─────────────────────────────────────────────────

  List<Node> _parseNodes(String? parentTag) {
    final nodes = <Node>[];
    while (!_eof) {
      // Preserve inter-element whitespace as collapsed text nodes
      // so that inline elements retain spacing (e.g., <b>A</b> <b>B</b>).
      if (_isWhitespace(_current)) {
        _consumeWhile(_isWhitespace);
        // Add a single-space text node to preserve inline spacing.
        nodes.add(Text(' '));
        if (_eof) break;
      }

      // Check for closing tag for our parent.
      if (_startsWith('</')) {
        final saved = _pos;
        _advance(2);
        final tag = _consumeWhile((c) => c != '>' && !_isWhitespace(c)).toLowerCase();
        // Consume to end of tag.
        while (!_eof && _current != '>') _advance();
        if (!_eof) _advance();

        if (tag == parentTag) {
          break; // Matched our parent's close tag.
        }
        // Mismatched close tag — if it matches an ancestor, put pointer back
        // and let the parent handle it. Otherwise, just ignore it.
        if (parentTag != null && _isAncestorTag(parentTag, tag)) {
          _pos = saved;
          break;
        }
        // Ignore stray close tags.
        continue;
      }

      // Check for comment.
      if (_startsWith('<!--')) {
        _parseComment(); // Discard comments in Phase 1.
        continue;
      }

      // Check for opening tag — must start with < followed by a letter or /.
      if (_current == '<' && _peek(1) != '' &&
          (RegExp(r'[a-zA-Z/!?]').hasMatch(_peek(1)))) {
        // Peek at the tag name to check if it should auto-close the parent.
        if (parentTag != null && !_startsWith('</') && !_startsWith('<!')) {
          final saved = _pos;
          _advance(); // skip <
          final peekedTag = _consumeWhile(
            (c) => c != '>' && c != '/' && !_isWhitespace(c),
          ).toLowerCase();
          _pos = saved; // restore position
          if (peekedTag.isNotEmpty && _shouldAutoClose(parentTag, peekedTag)) {
            break; // Auto-close parent; let the grandparent handle this tag.
          }
        }
        final element = _parseElement();
        if (element != null) {
          nodes.add(element);
        }
        continue;
      }

      // Text node.
      final text = _parseText();
      if (text.data.isNotEmpty) {
        nodes.add(text);
      }
    }
    return nodes;
  }

  /// Simple heuristic: some tags auto-close their siblings.
  bool _isAncestorTag(String current, String closing) {
    // If closing is a structural tag, likely means our parent should close.
    const structural = {
      'html', 'head', 'body', 'div', 'section', 'article',
      'main', 'nav', 'aside', 'header', 'footer', 'table',
      'ul', 'ol', 'dl', 'form', 'fieldset', 'select',
      'details', 'dialog', 'menu', 'blockquote', 'figure',
    };
    return structural.contains(closing);
  }

  /// HTML spec: certain tags implicitly close their parent when opened.
  bool _shouldAutoClose(String parent, String child) {
    switch (parent) {
      case 'p':
        // <p> auto-closes when a block element opens inside it.
        return blockElements.contains(child) || child == 'p';
      case 'li':
        return child == 'li';
      case 'td':
        return child == 'td' || child == 'th';
      case 'th':
        return child == 'td' || child == 'th';
      case 'dt':
        return child == 'dt' || child == 'dd';
      case 'dd':
        return child == 'dt' || child == 'dd';
      case 'tr':
        return child == 'tr';
      case 'thead':
        return child == 'tbody' || child == 'tfoot';
      case 'tbody':
        return child == 'thead' || child == 'tbody' || child == 'tfoot';
      case 'tfoot':
        return child == 'thead' || child == 'tbody';
      case 'option':
        return child == 'option' || child == 'optgroup';
      case 'optgroup':
        return child == 'optgroup';
      case 'head':
        return child == 'body';
      default:
        return false;
    }
  }

  Text _parseText() {
    final text = _consumeWhile((c) => c != '<');
    // Collapse whitespace runs into single spaces.
    final collapsed = text.replaceAll(RegExp(r'\s+'), ' ');
    return Text(_decodeEntities(collapsed));
  }

  void _parseComment() {
    _advance(4); // skip <!--
    while (!_eof && !_startsWith('-->')) {
      _advance();
    }
    if (!_eof) _advance(3); // skip -->
  }

  Element? _parseElement() {
    _advance(); // skip <
    final tagName = _consumeWhile(
      (c) => c != '>' && c != '/' && !_isWhitespace(c),
    ).toLowerCase();

    if (tagName.isEmpty) {
      // Malformed tag, skip.
      _consumeWhile((c) => c != '>');
      if (!_eof) _advance();
      return null;
    }

    final attributes = _parseAttributes();

    // Self-closing />
    bool selfClosing = false;
    _skipWhitespace();
    if (!_eof && _current == '/') {
      selfClosing = true;
      _advance();
    }
    // Skip >
    if (!_eof && _current == '>') _advance();

    final element = Element(tagName, attributes);

    // Void elements never have children.
    if (selfClosing || voidElements.contains(tagName)) {
      return element;
    }

    // Raw text elements: <style>, <title>, <textarea> — read until close tag.
    if (_isRawTextElement(tagName)) {
      final content = _consumeRawContent(tagName);
      if (content.isNotEmpty) {
        element.appendChild(Text(content));
      }
      return element;
    }

    // Ignored elements: skip their content entirely.
    if (ignoredElements.contains(tagName)) {
      _consumeRawContent(tagName); // Discard content.
      return null;
    }

    // Parse children.
    final children = _parseNodes(tagName);
    for (final child in children) {
      element.appendChild(child);
    }

    return element;
  }

  bool _isRawTextElement(String tag) =>
      tag == 'style' || tag == 'title' || tag == 'textarea';

  String _consumeRawContent(String tag) {
    final endTag = '</$tag>';
    final buf = StringBuffer();
    while (!_eof) {
      if (_startsWith(endTag) ||
          _startsWith(endTag.replaceAll(tag, tag.toUpperCase()))) {
        // Skip the closing tag.
        _advance(endTag.length);
        break;
      }
      // Also handle case-insensitive matching.
      if (_startsWith('</')) {
        final saved = _pos;
        _advance(2);
        final closeName = _consumeWhile((c) => c != '>' && !_isWhitespace(c));
        if (closeName.toLowerCase() == tag) {
          while (!_eof && _current != '>') _advance();
          if (!_eof) _advance();
          break;
        }
        // Not our close tag, put it back as content.
        final consumed = _input.substring(saved, _pos);
        buf.write(consumed);
        continue;
      }
      buf.write(_current);
      _advance();
    }
    return buf.toString();
  }

  Map<String, String> _parseAttributes() {
    final attrs = <String, String>{};
    while (!_eof) {
      _skipWhitespace();
      if (_eof || _current == '>' || _current == '/') break;

      // Attribute name.
      final name = _consumeWhile(
        (c) => c != '=' && c != '>' && c != '/' && !_isWhitespace(c),
      ).toLowerCase();

      if (name.isEmpty) {
        _advance(); // Skip unexpected character.
        continue;
      }

      _skipWhitespace();

      // Attribute value.
      if (!_eof && _current == '=') {
        _advance(); // skip =
        _skipWhitespace();

        String value;
        if (!_eof && (_current == '"' || _current == "'")) {
          final quote = _current;
          _advance();
          value = _consumeWhile((c) => c != quote);
          if (!_eof) _advance(); // skip closing quote
        } else {
          // Unquoted value.
          value = _consumeWhile(
            (c) => c != '>' && c != '/' && !_isWhitespace(c),
          );
        }
        attrs[name] = _decodeEntities(value);
      } else {
        // Boolean attribute (e.g., "disabled").
        attrs[name] = '';
      }
    }
    return attrs;
  }

  /// Named HTML entities → Unicode code points.
  static const _namedEntities = <String, String>{
    // XML predefined
    'amp': '&', 'lt': '<', 'gt': '>', 'quot': '"', 'apos': "'",
    // Whitespace / special
    'nbsp': '\u00A0', 'ensp': '\u2002', 'emsp': '\u2003',
    'thinsp': '\u2009', 'shy': '\u00AD',
    'zwnj': '\u200C', 'zwj': '\u200D',
    // Punctuation / typography
    'mdash': '\u2014', 'ndash': '\u2013', 'hellip': '\u2026',
    'lsquo': '\u2018', 'rsquo': '\u2019', 'sbquo': '\u201A',
    'ldquo': '\u201C', 'rdquo': '\u201D', 'bdquo': '\u201E',
    'laquo': '\u00AB', 'raquo': '\u00BB',
    'bull': '\u2022', 'middot': '\u00B7',
    'prime': '\u2032', 'Prime': '\u2033',
    // Symbols
    'copy': '\u00A9', 'reg': '\u00AE', 'trade': '\u2122',
    'times': '\u00D7', 'divide': '\u00F7',
    'plusmn': '\u00B1', 'minus': '\u2212',
    'deg': '\u00B0', 'micro': '\u00B5', 'permil': '\u2030',
    // Fractions
    'frac14': '\u00BC', 'frac12': '\u00BD', 'frac34': '\u00BE',
    // Currency
    'euro': '\u20AC', 'pound': '\u00A3', 'yen': '\u00A5', 'cent': '\u00A2',
    'curren': '\u00A4',
    // Typographic marks
    'sect': '\u00A7', 'para': '\u00B6', 'dagger': '\u2020', 'Dagger': '\u2021',
    'loz': '\u25CA', 'spades': '\u2660', 'clubs': '\u2663',
    'hearts': '\u2665', 'diams': '\u2666',
    // Arrows
    'larr': '\u2190', 'uarr': '\u2191', 'rarr': '\u2192', 'darr': '\u2193',
    'harr': '\u2194', 'crarr': '\u21B5',
    // Math
    'sum': '\u2211', 'prod': '\u220F', 'infin': '\u221E',
    'radic': '\u221A', 'asymp': '\u2248', 'ne': '\u2260',
    'le': '\u2264', 'ge': '\u2265',
    'and': '\u2227', 'or': '\u2228', 'not': '\u00AC',
    'empty': '\u2205', 'isin': '\u2208', 'notin': '\u2209',
    'sub': '\u2282', 'sup': '\u2283',
    // Greek (commonly used)
    'Alpha': '\u0391', 'Beta': '\u0392', 'Gamma': '\u0393', 'Delta': '\u0394',
    'Epsilon': '\u0395', 'Theta': '\u0398', 'Lambda': '\u039B', 'Pi': '\u03A0',
    'Sigma': '\u03A3', 'Omega': '\u03A9',
    'alpha': '\u03B1', 'beta': '\u03B2', 'gamma': '\u03B3', 'delta': '\u03B4',
    'epsilon': '\u03B5', 'theta': '\u03B8', 'lambda': '\u03BB', 'mu': '\u03BC',
    'pi': '\u03C0', 'sigma': '\u03C3', 'tau': '\u03C4', 'omega': '\u03C9',
    // Miscellaneous
    'iexcl': '\u00A1', 'iquest': '\u00BF', 'ordf': '\u00AA', 'ordm': '\u00BA',
    'macr': '\u00AF', 'acute': '\u00B4', 'cedil': '\u00B8',
    'circ': '\u02C6', 'tilde': '\u02DC',
  };

  /// Decode HTML entities (named + numeric).
  String _decodeEntities(String s) {
    return s.replaceAllMapped(RegExp(r'&(#x?[0-9a-fA-F]+|[a-zA-Z][a-zA-Z0-9]*);'), (m) {
      final entity = m.group(1)!;
      // Numeric: decimal &#NNN; or hex &#xHHH;
      if (entity.startsWith('#x') || entity.startsWith('#X')) {
        final code = int.tryParse(entity.substring(2), radix: 16);
        return code != null ? String.fromCharCode(code) : m.group(0)!;
      }
      if (entity.startsWith('#')) {
        final code = int.tryParse(entity.substring(1));
        return code != null ? String.fromCharCode(code) : m.group(0)!;
      }
      // Named entity lookup.
      return _namedEntities[entity] ?? m.group(0)!;
    });
  }
}
