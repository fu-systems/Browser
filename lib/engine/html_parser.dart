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
    if (_startsWith('<!') || _startsWith('<!')) {
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
      _skipWhitespace();
      if (_eof) break;

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

      // Check for opening tag.
      if (_current == '<' && _peek(1) != '' && _peek(1) != ' ') {
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
      'ul', 'ol', 'dl', 'form', 'fieldset',
    };
    return structural.contains(closing);
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

  /// Decode HTML entities (named + numeric).
  String _decodeEntities(String s) {
    return s
        .replaceAll('&amp;', '&')
        .replaceAll('&lt;', '<')
        .replaceAll('&gt;', '>')
        .replaceAll('&quot;', '"')
        .replaceAll('&#39;', "'")
        .replaceAll('&apos;', "'")
        .replaceAll('&nbsp;', '\u00A0')
        .replaceAllMapped(RegExp(r'&#(\d+);'), (m) {
          final code = int.tryParse(m.group(1)!);
          return code != null ? String.fromCharCode(code) : m.group(0)!;
        })
        .replaceAllMapped(RegExp(r'&#x([0-9a-fA-F]+);'), (m) {
          final code = int.tryParse(m.group(1)!, radix: 16);
          return code != null ? String.fromCharCode(code) : m.group(0)!;
        });
  }
}
