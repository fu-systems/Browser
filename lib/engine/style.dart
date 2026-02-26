/// Style resolution — pure Dart, no Flutter dependency.
///
/// Matches CSS selectors to DOM elements, computes specificity,
/// cascades declarations, and produces a StyledNode tree with
/// resolved property values.

import 'dom.dart';
import 'css.dart';

/// A DOM node annotated with computed CSS property values.
class StyledNode {
  final Node node;
  final Map<String, String> properties;
  final List<StyledNode> children;

  StyledNode(this.node, this.properties, this.children);

  String? operator [](String property) => properties[property];

  /// Convenience: look up a property, falling back to a default.
  String prop(String name, [String fallback = '']) =>
      properties[name] ?? fallback;

  /// Resolve the display type.
  Display get display {
    final d = prop('display');
    if (d == 'none') return Display.none;
    if (d == 'inline') return Display.inline;
    if (d == 'block') return Display.block;
    if (d == 'inline-block') return Display.inlineBlock;
    // Default: block for block elements, inline otherwise.
    if (node is Element) {
      return blockElements.contains((node as Element).tagName)
          ? Display.block
          : Display.inline;
    }
    return Display.inline;
  }
}

enum Display { block, inline, inlineBlock, none }

// ── Style computation ───────────────────────────────────────────────

/// Build a StyledNode tree from a DOM tree and a list of stylesheets.
StyledNode computeStyles(Node node, List<Stylesheet> stylesheets) {
  return _styleNode(node, stylesheets, {});
}

StyledNode _styleNode(
  Node node,
  List<Stylesheet> stylesheets,
  Map<String, String> inherited,
) {
  Map<String, String> props;

  if (node is Element) {
    props = _resolveElement(node, stylesheets, inherited);
  } else {
    // Text nodes inherit from their parent.
    props = Map.of(inherited);
  }

  final children = <StyledNode>[];
  for (final child in node.children) {
    if (child is Element && child.tagName == 'head') continue; // Skip head.
    if (child is Comment) continue;
    final styledChild = _styleNode(child, stylesheets, _inheritableProps(props));
    children.add(styledChild);
  }

  return StyledNode(node, props, children);
}

/// Resolve all properties for an Element by cascading:
/// 1. User-agent defaults
/// 2. Stylesheet rules (ordered by specificity)
/// 3. Inline styles (highest specificity)
/// 4. Inherited properties from parent
Map<String, String> _resolveElement(
  Element element,
  List<Stylesheet> stylesheets,
  Map<String, String> inherited,
) {
  // Start with inherited properties.
  final props = Map<String, String>.from(inherited);

  // Apply user-agent defaults (override inherited values for this element).
  _applyDefaults(element, props);

  // Collect all matching rules with specificity.
  final matches = <(Specificity, Map<String, String>)>[];

  for (final sheet in stylesheets) {
    for (final rule in sheet.rules) {
      if (_selectorMatches(element, rule.selector)) {
        matches.add((rule.selector.specificity, rule.declarations));
      }
    }
  }

  // Sort by specificity (ascending), then apply in order.
  matches.sort((a, b) => a.$1.compareTo(b.$1));
  for (final (_, decls) in matches) {
    props.addAll(decls);
  }

  // Apply inline styles (highest specificity).
  if (element.inlineStyle.isNotEmpty) {
    props.addAll(CssParser.parseInlineStyle(element.inlineStyle));
  }

  return props;
}

/// Check if [selector] matches [element].
bool _selectorMatches(Element element, Selector selector) {
  // Check the simple part (tag, id, classes).
  if (selector.tag != null && selector.tag != element.tagName) return false;
  if (selector.id != null && selector.id != element.id) return false;
  for (final cls in selector.classes) {
    if (!element.classes.contains(cls)) return false;
  }

  // Check ancestor (descendant combinator).
  if (selector.ancestor != null) {
    Node? parent = element.parent;
    while (parent != null) {
      if (parent is Element && _selectorMatches(parent, selector.ancestor!)) {
        return true;
      }
      parent = parent.parent;
    }
    return false; // No ancestor matched.
  }

  return true;
}

/// Properties that inherit from parent to child.
const _inheritableProperties = {
  'color',
  'font-family',
  'font-size',
  'font-weight',
  'font-style',
  'line-height',
  'text-align',
  'text-decoration',
  'text-transform',
  'letter-spacing',
  'word-spacing',
  'white-space',
  'visibility',
  'cursor',
  'list-style',
  'list-style-type',
};

Map<String, String> _inheritableProps(Map<String, String> props) {
  return {
    for (final key in _inheritableProperties)
      if (props.containsKey(key)) key: props[key]!,
  };
}

/// Apply default styles based on the element tag — our minimal user-agent stylesheet.
void _applyDefaults(Element element, Map<String, String> props) {
  // Tag-specific UA defaults override inherited values (e.g. h1 overrides
  // the body font-size it inherited). Using direct assignment, not putIfAbsent.
  switch (element.tagName) {
    case 'h1':
      props['font-size'] = '32px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '21px';
      props['margin-bottom'] = '21px';
    case 'h2':
      props['font-size'] = '24px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '19px';
      props['margin-bottom'] = '19px';
    case 'h3':
      props['font-size'] = '19px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '18px';
      props['margin-bottom'] = '18px';
    case 'h4':
      props['font-size'] = '16px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '21px';
      props['margin-bottom'] = '21px';
    case 'h5':
      props['font-size'] = '13px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '22px';
      props['margin-bottom'] = '22px';
    case 'h6':
      props['font-size'] = '11px';
      props['font-weight'] = 'bold';
      props['margin-top'] = '25px';
      props['margin-bottom'] = '25px';
    case 'p':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
    case 'a':
      props['color'] = '#0000EE';
      props['text-decoration'] = 'underline';
      props['cursor'] = 'pointer';
    case 'strong' || 'b':
      props['font-weight'] = 'bold';
    case 'em' || 'i':
      props['font-style'] = 'italic';
    case 'u':
      props['text-decoration'] = 'underline';
    case 'code':
      props['font-family'] = 'monospace';
    case 'pre':
      props['font-family'] = 'monospace';
      props['white-space'] = 'pre';
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
    case 'blockquote':
      props['margin-left'] = '40px';
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
    case 'ul' || 'ol':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
      props['padding-left'] = '40px';
    case 'li':
      props['display'] = 'block';
    case 'hr':
      props['margin-top'] = '8px';
      props['margin-bottom'] = '8px';
      props['border-top'] = '1px solid #808080';
    case 'body':
      props.putIfAbsent('font-family', () => 'serif');
      props.putIfAbsent('font-size', () => '16px');
      props.putIfAbsent('color', () => '#000000');
      props.putIfAbsent('margin', () => '8px');
    case 'center':
      props['text-align'] = 'center';
    case 'tt':
      props['font-family'] = 'monospace';
    case 'font':
      final face = element.attributes['face'];
      if (face != null && face.isNotEmpty) {
        props['font-family'] = face.split(',').first.trim();
      }
      final size = element.attributes['size'];
      if (size != null && size.isNotEmpty) {
        props['font-size'] = _htmlFontSize(size);
      }
    case 'table':
      props['display'] = 'block';
      props['border-collapse'] = 'separate';
    case 'td' || 'th':
      props['padding'] = '1px';
    case 'th':
      props['font-weight'] = 'bold';
      props['text-align'] = 'center';
  }

  // Apply HTML width/height attributes as presentational hints.
  _applyHtmlAttributes(element, props);
}

void _applyHtmlAttributes(Element element, Map<String, String> props) {
  final w = element.attributes['width'];
  if (w != null && w.isNotEmpty && !props.containsKey('width')) {
    props['width'] = _cssifyDimension(w);
  }
  final h = element.attributes['height'];
  if (h != null && h.isNotEmpty && !props.containsKey('height')) {
    props['height'] = _cssifyDimension(h);
  }
}

String _cssifyDimension(String value) {
  if (value.endsWith('%') || value.endsWith('px') || value.endsWith('em')) {
    return value;
  }
  if (double.tryParse(value) != null) return '${value}px';
  return value;
}

String _htmlFontSize(String size) {
  int absolute;
  if (size.startsWith('+')) {
    absolute = 3 + (int.tryParse(size.substring(1)) ?? 0);
  } else if (size.startsWith('-')) {
    absolute = 3 - (int.tryParse(size.substring(1)) ?? 0);
  } else {
    absolute = int.tryParse(size) ?? 3;
  }
  absolute = absolute.clamp(1, 7);
  const sizeMap = {
    1: '10px', 2: '13px', 3: '16px', 4: '18px',
    5: '24px', 6: '32px', 7: '48px',
  };
  return sizeMap[absolute] ?? '16px';
}
