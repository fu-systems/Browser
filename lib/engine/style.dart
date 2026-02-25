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

  // Apply user-agent defaults.
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
  switch (element.tagName) {
    case 'h1':
      props.putIfAbsent('font-size', () => '32px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '21px');
      props.putIfAbsent('margin-bottom', () => '21px');
    case 'h2':
      props.putIfAbsent('font-size', () => '24px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '19px');
      props.putIfAbsent('margin-bottom', () => '19px');
    case 'h3':
      props.putIfAbsent('font-size', () => '19px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '18px');
      props.putIfAbsent('margin-bottom', () => '18px');
    case 'h4':
      props.putIfAbsent('font-size', () => '16px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '21px');
      props.putIfAbsent('margin-bottom', () => '21px');
    case 'h5':
      props.putIfAbsent('font-size', () => '13px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '22px');
      props.putIfAbsent('margin-bottom', () => '22px');
    case 'h6':
      props.putIfAbsent('font-size', () => '11px');
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('margin-top', () => '25px');
      props.putIfAbsent('margin-bottom', () => '25px');
    case 'p':
      props.putIfAbsent('margin-top', () => '16px');
      props.putIfAbsent('margin-bottom', () => '16px');
    case 'a':
      props.putIfAbsent('color', () => '#0000EE');
      props.putIfAbsent('text-decoration', () => 'underline');
      props.putIfAbsent('cursor', () => 'pointer');
    case 'strong' || 'b':
      props.putIfAbsent('font-weight', () => 'bold');
    case 'em' || 'i':
      props.putIfAbsent('font-style', () => 'italic');
    case 'u':
      props.putIfAbsent('text-decoration', () => 'underline');
    case 'code':
      props.putIfAbsent('font-family', () => 'monospace');
    case 'pre':
      props.putIfAbsent('font-family', () => 'monospace');
      props.putIfAbsent('white-space', () => 'pre');
      props.putIfAbsent('margin-top', () => '16px');
      props.putIfAbsent('margin-bottom', () => '16px');
    case 'blockquote':
      props.putIfAbsent('margin-left', () => '40px');
      props.putIfAbsent('margin-top', () => '16px');
      props.putIfAbsent('margin-bottom', () => '16px');
    case 'ul' || 'ol':
      props.putIfAbsent('margin-top', () => '16px');
      props.putIfAbsent('margin-bottom', () => '16px');
      props.putIfAbsent('padding-left', () => '40px');
    case 'li':
      props.putIfAbsent('display', () => 'block');
    case 'hr':
      props.putIfAbsent('margin-top', () => '8px');
      props.putIfAbsent('margin-bottom', () => '8px');
      props.putIfAbsent('border-top', () => '1px solid #808080');
    case 'body':
      props.putIfAbsent('font-family', () => 'serif');
      props.putIfAbsent('font-size', () => '16px');
      props.putIfAbsent('color', () => '#000000');
      props.putIfAbsent('margin', () => '8px');
    case 'table':
      props.putIfAbsent('display', () => 'block');
      props.putIfAbsent('border-collapse', () => 'separate');
    case 'td' || 'th':
      props.putIfAbsent('padding', () => '1px');
    case 'th':
      props.putIfAbsent('font-weight', () => 'bold');
      props.putIfAbsent('text-align', () => 'center');
  }
}
