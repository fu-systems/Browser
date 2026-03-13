/// DOM tree representation — pure Dart, no Flutter dependency.
///
/// Models the DOM needed by Pane: Document, Element, Text, and Comment nodes.
/// Includes sibling/index helpers required by CSS selector matching.

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

  /// Previous sibling node.
  Node? get previousSibling {
    if (parent == null) return null;
    final siblings = parent!.children;
    final idx = siblings.indexOf(this);
    return idx > 0 ? siblings[idx - 1] : null;
  }

  /// Next sibling node.
  Node? get nextSibling {
    if (parent == null) return null;
    final siblings = parent!.children;
    final idx = siblings.indexOf(this);
    return idx >= 0 && idx < siblings.length - 1 ? siblings[idx + 1] : null;
  }
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

  /// Dynamic pseudo-class state — set by the UI layer.
  bool isHovered = false;
  bool isFocused = false;
  bool isActive = false;

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

  // ── Sibling / index helpers for CSS selectors ──

  /// All Element siblings of this element (including self).
  List<Element> get elementSiblings {
    if (parent == null) return [this];
    return parent!.children.whereType<Element>().toList();
  }

  /// 1-based index among element siblings.
  int get elementIndex {
    final sibs = elementSiblings;
    return sibs.indexOf(this) + 1;
  }

  /// 1-based index counting from the end.
  int get elementIndexFromEnd {
    final sibs = elementSiblings;
    return sibs.length - sibs.indexOf(this);
  }

  /// 1-based index among element siblings of the same tag name.
  int get elementIndexOfType {
    final sibs = elementSiblings.where((e) => e.tagName == tagName).toList();
    return sibs.indexOf(this) + 1;
  }

  /// 1-based index from end among element siblings of the same tag name.
  int get elementIndexOfTypeFromEnd {
    final sibs = elementSiblings.where((e) => e.tagName == tagName).toList();
    return sibs.length - sibs.indexOf(this);
  }

  /// Whether this element is the only Element child of its parent.
  bool get isOnlyChild => elementSiblings.length == 1;

  /// Whether this is the only child of its type among siblings.
  bool get isOnlyOfType =>
      elementSiblings.where((e) => e.tagName == tagName).length == 1;

  /// Previous Element sibling (skipping text/comment nodes).
  Element? get previousElementSibling {
    if (parent == null) return null;
    final sibs = elementSiblings;
    final idx = sibs.indexOf(this);
    return idx > 0 ? sibs[idx - 1] : null;
  }

  /// Next Element sibling (skipping text/comment nodes).
  Element? get nextElementSibling {
    if (parent == null) return null;
    final sibs = elementSiblings;
    final idx = sibs.indexOf(this);
    return idx >= 0 && idx < sibs.length - 1 ? sibs[idx + 1] : null;
  }

  /// Whether this element has any child text content (non-whitespace).
  bool get hasTextContent => textContent.trim().isNotEmpty;

  /// Whether this element has no children at all.
  bool get isEmpty => children.isEmpty;

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
const ignoredElements = {'script', 'template'};

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
