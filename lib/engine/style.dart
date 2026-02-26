/// Style resolution — pure Dart, no Flutter dependency.
///
/// Matches CSS Selectors Level 4 to DOM elements, computes specificity,
/// cascades declarations with !important support, and produces a
/// StyledNode tree with resolved property values.

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
    if (d == 'flex') return Display.flex;
    if (d == 'inline-flex') return Display.inlineFlex;
    // Default: block for block elements, inline otherwise.
    if (node is Element) {
      return blockElements.contains((node as Element).tagName)
          ? Display.block
          : Display.inline;
    }
    return Display.inline;
  }
}

enum Display { block, inline, inlineBlock, none, flex, inlineFlex }

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
/// 2. Stylesheet rules (ordered by specificity), normal declarations
/// 3. Inline styles (highest specificity for normal)
/// 4. !important declarations (override everything except inline !important)
/// 5. Inherited properties from parent
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
  final matches = <(Specificity, Map<String, CssValue>)>[];

  for (final sheet in stylesheets) {
    for (final rule in sheet.rules) {
      if (_selectorMatches(element, rule.selector)) {
        matches.add((rule.selector.specificity, rule.declarations));
      }
    }
  }

  // Sort by specificity (ascending), then apply in order.
  matches.sort((a, b) => a.$1.compareTo(b.$1));

  // First pass: apply normal (non-important) declarations.
  for (final (_, decls) in matches) {
    for (final entry in decls.entries) {
      if (!entry.value.important) {
        props[entry.key] = entry.value.value;
      }
    }
  }

  // Apply inline styles (highest specificity for normal declarations).
  if (element.inlineStyle.isNotEmpty) {
    final inlineDecls = CssParser.parseInlineStyle(element.inlineStyle);
    for (final entry in inlineDecls.entries) {
      if (!entry.value.important) {
        props[entry.key] = entry.value.value;
      }
    }
  }

  // Second pass: apply !important declarations (override everything).
  for (final (_, decls) in matches) {
    for (final entry in decls.entries) {
      if (entry.value.important) {
        props[entry.key] = entry.value.value;
      }
    }
  }

  // Inline !important overrides even stylesheet !important.
  if (element.inlineStyle.isNotEmpty) {
    final inlineDecls = CssParser.parseInlineStyle(element.inlineStyle);
    for (final entry in inlineDecls.entries) {
      if (entry.value.important) {
        props[entry.key] = entry.value.value;
      }
    }
  }

  // Handle CSS-wide keywords: inherit, initial, unset.
  _resolveKeywords(props, inherited);

  return props;
}

/// Handle inherit/initial/unset keywords.
void _resolveKeywords(Map<String, String> props, Map<String, String> inherited) {
  final keys = props.keys.toList();
  for (final key in keys) {
    final val = props[key];
    if (val == 'inherit') {
      if (inherited.containsKey(key)) {
        props[key] = inherited[key]!;
      } else {
        props.remove(key);
      }
    } else if (val == 'initial') {
      props.remove(key); // Revert to browser default (no value).
    } else if (val == 'unset') {
      if (_inheritableProperties.contains(key)) {
        // Inheritable: acts like inherit.
        if (inherited.containsKey(key)) {
          props[key] = inherited[key]!;
        } else {
          props.remove(key);
        }
      } else {
        // Non-inheritable: acts like initial.
        props.remove(key);
      }
    }
  }
}

// ── Selector matching (CSS Selectors Level 4) ───────────────────────

/// Check if [selector] matches [element].
bool _selectorMatches(Element element, Selector selector) {
  if (selector.compounds.isEmpty) return false;

  // compounds[0] is the subject (rightmost), compounds[1..n] are ancestors/siblings.
  // combinators[0] connects compounds[0] to compounds[1], etc.
  if (!_compoundMatches(element, selector.compounds[0])) return false;

  // Walk the combinator chain.
  Element? current = element;
  for (int i = 0; i < selector.combinators.length; i++) {
    if (current == null) return false;
    final combinator = selector.combinators[i];
    final compound = selector.compounds[i + 1];

    switch (combinator) {
      case Combinator.descendant:
        // Any ancestor must match.
        Node? ancestor = current.parent;
        bool found = false;
        while (ancestor != null) {
          if (ancestor is Element && _compoundMatches(ancestor, compound)) {
            current = ancestor;
            found = true;
            break;
          }
          ancestor = ancestor.parent;
        }
        if (!found) return false;

      case Combinator.child:
        // Direct parent must match.
        final parent = current.parent;
        if (parent is! Element || !_compoundMatches(parent, compound)) return false;
        current = parent;

      case Combinator.adjacentSibling:
        // Previous element sibling must match.
        final prev = current.previousElementSibling;
        if (prev == null || !_compoundMatches(prev, compound)) return false;
        current = prev;

      case Combinator.generalSibling:
        // Any preceding element sibling must match.
        final siblings = current.elementSiblings;
        final idx = siblings.indexOf(current);
        bool found = false;
        for (int j = idx - 1; j >= 0; j--) {
          if (_compoundMatches(siblings[j], compound)) {
            current = siblings[j];
            found = true;
            break;
          }
        }
        if (!found) return false;
    }
  }

  return true;
}

/// Check if a compound selector matches an element.
bool _compoundMatches(Element element, CompoundSelector compound) {
  // Tag check.
  if (compound.tag != null && compound.tag != element.tagName) return false;

  // ID check.
  if (compound.id != null && compound.id != element.id) return false;

  // Class checks.
  for (final cls in compound.classes) {
    if (!element.classes.contains(cls)) return false;
  }

  // Attribute checks.
  for (final attr in compound.attributes) {
    if (!_attributeMatches(element, attr)) return false;
  }

  // Pseudo-class checks.
  for (final pseudo in compound.pseudoClasses) {
    if (!_pseudoClassMatches(element, pseudo)) return false;
  }

  return true;
}

/// Check if an attribute selector matches.
bool _attributeMatches(Element element, AttributeSelector attr) {
  final value = element.attributes[attr.name];

  if (attr.op == null) {
    // [attr] — just check presence.
    return element.attributes.containsKey(attr.name);
  }

  if (value == null) return false;

  String v = value;
  String? target = attr.value;
  if (attr.caseInsensitive && target != null) {
    v = v.toLowerCase();
    target = target.toLowerCase();
  }

  switch (attr.op) {
    case '=':
      return v == target;
    case '~=':
      return v.split(RegExp(r'\s+')).contains(target);
    case '|=':
      return v == target || v.startsWith('${target!}-');
    case '^=':
      return target != null && v.startsWith(target);
    case '\$=':
      return target != null && v.endsWith(target);
    case '*=':
      return target != null && v.contains(target);
    default:
      return false;
  }
}

/// Check if a pseudo-class matches.
bool _pseudoClassMatches(Element element, PseudoSelector pseudo) {
  switch (pseudo.name) {
    case 'first-child':
      return element.elementIndex == 1;
    case 'last-child':
      return element.elementIndexFromEnd == 1;
    case 'first-of-type':
      return element.elementIndexOfType == 1;
    case 'last-of-type':
      return element.elementIndexOfTypeFromEnd == 1;
    case 'only-child':
      return element.isOnlyChild;
    case 'only-of-type':
      return element.isOnlyOfType;
    case 'empty':
      return element.isEmpty;
    case 'root':
      return element.parent is Document;
    case 'link':
      return element.tagName == 'a' && element.attributes.containsKey('href');
    case 'visited':
      return false; // We don't track visited links.
    case 'hover':
    case 'active':
    case 'focus':
    case 'focus-within':
    case 'focus-visible':
      return false; // Dynamic states — not matched during initial styling.
    case 'enabled':
      return !element.attributes.containsKey('disabled');
    case 'disabled':
      return element.attributes.containsKey('disabled');
    case 'checked':
      return element.attributes.containsKey('checked');
    case 'required':
      return element.attributes.containsKey('required');
    case 'optional':
      return !element.attributes.containsKey('required');
    case 'read-only':
      return element.attributes.containsKey('readonly');
    case 'read-write':
      return !element.attributes.containsKey('readonly');

    case 'nth-child':
      return _nthMatches(element.elementIndex, pseudo.argument ?? '');
    case 'nth-last-child':
      return _nthMatches(element.elementIndexFromEnd, pseudo.argument ?? '');
    case 'nth-of-type':
      return _nthMatches(element.elementIndexOfType, pseudo.argument ?? '');
    case 'nth-last-of-type':
      return _nthMatches(element.elementIndexOfTypeFromEnd, pseudo.argument ?? '');

    case 'not':
      if (pseudo.selectorArg != null) {
        return !_selectorMatches(element, pseudo.selectorArg!);
      }
      return true;
    case 'is':
    case 'matches':
    case 'where':
      if (pseudo.selectorArg != null) {
        return _selectorMatches(element, pseudo.selectorArg!);
      }
      return false;
    case 'has':
      if (pseudo.selectorArg != null) {
        // :has() checks if any descendant matches.
        for (final desc in element.elementDescendants) {
          if (_selectorMatches(desc, pseudo.selectorArg!)) return true;
        }
      }
      return false;

    default:
      return true; // Unknown pseudo-classes pass (graceful degradation).
  }
}

/// Evaluate an An+B expression against an index (1-based).
bool _nthMatches(int index, String expression) {
  final expr = expression.trim().toLowerCase();
  if (expr == 'odd') return index % 2 == 1;
  if (expr == 'even') return index % 2 == 0;

  // Try simple number.
  final simple = int.tryParse(expr);
  if (simple != null) return index == simple;

  // Parse An+B form.
  final match = RegExp(r'^([+-]?\d*)n\s*([+-]\s*\d+)?$').firstMatch(expr);
  if (match == null) return false;

  final aStr = match.group(1) ?? '';
  final bStr = (match.group(2) ?? '').replaceAll(' ', '');

  int a;
  if (aStr.isEmpty || aStr == '+') {
    a = 1;
  } else if (aStr == '-') {
    a = -1;
  } else {
    a = int.tryParse(aStr) ?? 0;
  }
  final b = int.tryParse(bStr) ?? 0;

  if (a == 0) return index == b;
  final diff = index - b;
  if (diff == 0) return true;
  return diff % a == 0 && diff ~/ a >= 0;
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
  'direction',
  'text-indent',
  'quotes',
  'orphans',
  'widows',
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
    // ── Inline text semantics ──
    case 'strong' || 'b':
      props['font-weight'] = 'bold';
    case 'em' || 'i' || 'cite' || 'var' || 'dfn':
      props['font-style'] = 'italic';
    case 'u' || 'ins':
      props['text-decoration'] = 'underline';
    case 's' || 'del' || 'strike':
      props['text-decoration'] = 'line-through';
    case 'small':
      props['font-size'] = '13px';
    case 'big':
      props['font-size'] = '19px';
    case 'sub':
      props['vertical-align'] = 'sub';
      props['font-size'] = '13px';
    case 'sup':
      props['vertical-align'] = 'super';
      props['font-size'] = '13px';
    case 'mark':
      props['background-color'] = '#ffff00';
      props['color'] = '#000000';
    case 'abbr':
      props['text-decoration'] = 'underline dotted';
    case 'code' || 'kbd' || 'samp':
      props['font-family'] = 'monospace';

    // ── Preformatted / quoted ──
    case 'pre':
      props['font-family'] = 'monospace';
      props['white-space'] = 'pre';
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
    case 'blockquote':
      props['margin-left'] = '40px';
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
    case 'address':
      props['font-style'] = 'italic';
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';

    // ── Lists ──
    case 'ul':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
      props['padding-left'] = '40px';
      props['list-style-type'] = 'disc';
    case 'ol':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
      props['padding-left'] = '40px';
      props['list-style-type'] = 'decimal';
    case 'menu':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
      props['padding-left'] = '40px';
      props['list-style-type'] = 'disc';
    case 'li':
      props['display'] = 'block';
    case 'dd':
      props['margin-left'] = '40px';

    // ── Separators / structure ──
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

    // ── Legacy formatting ──
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
      final color = element.attributes['color'];
      if (color != null && color.isNotEmpty) {
        props['color'] = color;
      }

    // ── Tables ──
    case 'table':
      props['display'] = 'block';
      props['border-collapse'] = 'separate';
    case 'td':
      props['padding'] = '1px';
    case 'th':
      props['padding'] = '1px';
      props['font-weight'] = 'bold';
      props['text-align'] = 'center';
    case 'caption':
      props['text-align'] = 'center';

    // ── Figures ──
    case 'figure':
      props['margin-top'] = '16px';
      props['margin-bottom'] = '16px';
      props['margin-left'] = '40px';
      props['margin-right'] = '40px';

    // ── Forms ──
    case 'fieldset':
      props['border'] = '2px groove #c0c0c0';
      props['padding'] = '8px 12px 10px';
      props['margin-left'] = '2px';
      props['margin-right'] = '2px';
    case 'legend':
      props['padding-left'] = '4px';
      props['padding-right'] = '4px';

    // ── Interactive ──
    case 'dialog':
      props['border'] = '1px solid #000000';
      props['padding'] = '16px';
      props['background-color'] = '#ffffff';
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

// blockElements is defined in dom.dart and imported above.
