/// DOM tree representation — pure Dart, no Flutter dependency.
///
/// Models the subset of the DOM that Pane needs for Phase 1:
/// Document, Element, and Text nodes.

enum NodeType { document, element, text, comment }

class Node {
  NodeType nodeType;
  Node? parent;
  final List<Node> children = [];

  Node(this.nodeType);

  void appendChild(Node child) {
    child.parent = this;
    children.add(child);
  }

  void removeChild(Node child) {
    child.parent = null;
    children.remove(child);
  }

  /// Walk every descendant depth-first.
  Iterable<Node> get descendants sync* {
    for (final child in children) {
      yield child;
      yield* child.descendants;
    }
  }

  /// Convenience: all Element descendants.
  Iterable<Element> get elementDescendants =>
      descendants.whereType<Element>();
}

class Document extends Node {
  Document() : super(NodeType.document);

  Element? get documentElement {
    for (final c in children) {
      if (c is Element) return c;
    }
    return null;
  }

  Element? get head =>
      documentElement?.children
          .whereType<Element>()
          .where((e) => e.tagName == 'head')
          .firstOrNull;

  Element? get body =>
      documentElement?.children
          .whereType<Element>()
          .where((e) => e.tagName == 'body')
          .firstOrNull;

  /// Collect all <style> text content.
  String get internalCSS {
    final buf = StringBuffer();
    for (final el in elementDescendants) {
      if (el.tagName == 'style') {
        buf.writeln(el.textContent);
      }
    }
    return buf.toString();
  }

  /// Collect all <link rel="stylesheet" href="...">.
  List<String> get externalStylesheetUrls {
    final urls = <String>[];
    for (final el in elementDescendants) {
      if (el.tagName == 'link' &&
          el.attributes['rel']?.toLowerCase() == 'stylesheet') {
        final href = el.attributes['href'];
        if (href != null && href.isNotEmpty) urls.add(href);
      }
    }
    return urls;
  }

  /// Get the page title.
  String get title {
    for (final el in elementDescendants) {
      if (el.tagName == 'title') return el.textContent.trim();
    }
    return '';
  }
}

class Element extends Node {
  final String tagName;
  final Map<String, String> attributes;

  /// Inline style attribute, parsed later by the CSS layer.
  String get inlineStyle => attributes['style'] ?? '';

  String get id => attributes['id'] ?? '';

  List<String> get classes {
    final cls = attributes['class'] ?? '';
    if (cls.isEmpty) return const [];
    return cls.split(RegExp(r'\s+')).where((s) => s.isNotEmpty).toList();
  }

  Element(this.tagName, [Map<String, String>? attributes])
      : attributes = attributes ?? {},
        super(NodeType.element);

  /// Concatenated text content of all descendant Text nodes.
  String get textContent {
    final buf = StringBuffer();
    for (final node in descendants) {
      if (node is Text) buf.write(node.data);
    }
    return buf.toString();
  }

  @override
  String toString() => '<$tagName>';
}

class Text extends Node {
  String data;

  Text(this.data) : super(NodeType.text);

  @override
  String toString() => 'Text("${data.length > 40 ? '${data.substring(0, 40)}...' : data}")';
}

class Comment extends Node {
  final String data;

  Comment(this.data) : super(NodeType.comment);
}

/// Tags that are self-closing (void elements) in HTML5.
const voidElements = {
  'area', 'base', 'br', 'col', 'embed', 'hr', 'img', 'input',
  'link', 'meta', 'param', 'source', 'track', 'wbr',
};

/// Tags whose content Pane ignores entirely for rendering.
const ignoredElements = {'script', 'noscript', 'template'};

/// Block-level elements (used by layout engine).
const blockElements = {
  'html', 'body', 'article', 'section', 'nav', 'aside',
  'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'hgroup',
  'p', 'div', 'main', 'header', 'footer', 'center', 'address',
  'ul', 'ol', 'li', 'dl', 'dt', 'dd', 'menu',
  'blockquote', 'pre', 'figure', 'figcaption',
  'table', 'thead', 'tbody', 'tfoot', 'tr', 'td', 'th', 'caption', 'colgroup',
  'form', 'fieldset', 'legend',
  'details', 'summary', 'dialog', 'search', 'hr',
};
