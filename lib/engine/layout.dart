/// Layout engine — pure Dart, no Flutter dependency.
///
/// Takes a StyledNode tree and produces a LayoutBox tree with computed
/// x, y, width, height for every box. Implements block, inline, flexbox
/// layout with the CSS box model (margin, padding, border, box-sizing,
/// min/max constraints, overflow, and positioning).
///
/// Text measurement is provided via an abstract [TextMeasurer] interface
/// so the engine stays independent of Flutter.

import 'dart:math' as math;
import 'dart:typed_data';
import 'dom.dart';
import 'style.dart';

// ── Text measurement abstraction ────────────────────────────────────

/// Measures text dimensions for a given font configuration.
/// The Flutter layer provides an implementation using TextPainter.
abstract class TextMeasurer {
  TextMetrics measureText(
    String text, {
    required double fontSize,
    required String fontFamily,
    required String fontWeight,
    required String fontStyle,
    required double maxWidth,
  });
}

class TextMetrics {
  final double width;
  final double height;
  final List<TextLine> lines;
  TextMetrics(this.width, this.height, this.lines);
}

class TextLine {
  final String text;
  final double width;
  final double height;
  final double baseline;
  TextLine(this.text, this.width, this.height, this.baseline);
}

// ── Layout box model ────────────────────────────────────────────────

class EdgeSizes {
  final double top, right, bottom, left;
  const EdgeSizes(this.top, this.right, this.bottom, this.left);
  static const zero = EdgeSizes(0, 0, 0, 0);
}

class Rect {
  double x, y, width, height;
  Rect(this.x, this.y, this.width, this.height);

  Rect expandedBy(EdgeSizes e) => Rect(
        x - e.left,
        y - e.top,
        width + e.left + e.right,
        height + e.top + e.bottom,
      );
}

enum LayoutType { block, inline, anonymous, text, flex, grid, table }

/// A node in the layout tree. Each box has a content rect plus
/// margin, border, and padding edges.
class LayoutBox {
  LayoutType layoutType;
  final StyledNode? styledNode;
  final List<LayoutBox> children = [];

  Rect content = Rect(0, 0, 0, 0);
  EdgeSizes margin = EdgeSizes.zero;
  EdgeSizes border = EdgeSizes.zero;
  EdgeSizes padding = EdgeSizes.zero;

  /// For text boxes.
  String? text;
  List<TextLine>? textLines;

  /// For link detection: the href if this box is inside an <a> tag.
  String? linkHref;

  /// For image boxes: the resolved image URL and fetched bytes.
  String? imageUrl;
  Uint8List? imageBytes;
  double imageWidth = 0;
  double imageHeight = 0;

  /// For form elements: tag + type info for painting and interaction.
  String? formTag;
  String? formType;
  String? formValue;
  String? formPlaceholder;
  String? formName;
  String? formAction;
  String? formMethod;
  bool formChecked = false;
  bool formFocused = false;
  List<String>? formOptions;       // For <select>: option labels
  List<String>? formOptionValues;  // For <select>: option values

  /// Float behavior.
  String float_ = 'none';  // none, left, right
  String clear = 'none';   // none, left, right, both

  /// Positioning data.
  String position = 'static';  // static, relative, absolute, fixed, sticky
  double? posTop, posRight, posBottom, posLeft;
  int zIndex = 0;

  /// Overflow behavior.
  String overflow = 'visible';

  /// Opacity.
  double opacity = 1.0;

  /// Border radius (for painting).
  double borderRadiusTL = 0;
  double borderRadiusTR = 0;
  double borderRadiusBL = 0;
  double borderRadiusBR = 0;

  /// Box shadow data (for painting).
  String? boxShadow;

  LayoutBox(this.layoutType, [this.styledNode]);

  Rect get paddingBox => content.expandedBy(padding);
  Rect get borderBox => paddingBox.expandedBy(border);
  Rect get marginBox => borderBox.expandedBy(margin);

  /// Flatten all boxes depth-first for painting.
  Iterable<LayoutBox> get allBoxes sync* {
    yield this;
    for (final child in children) {
      yield* child.allBoxes;
    }
  }
}

// ── Layout algorithm ────────────────────────────────────────────────

/// Entry point: lay out a StyledNode tree into the given viewport.
LayoutBox layoutTree(
  StyledNode root,
  double viewportWidth,
  TextMeasurer measurer,
) {
  final rootBox = _buildLayoutTree(root, null);
  rootBox.content.width = viewportWidth;
  // Compute root dimensions to get margin/border/padding, then position
  // the content area inside them (no parent does this for the root box).
  _computeBoxDimensions(rootBox, viewportWidth);
  rootBox.content.x = rootBox.margin.left + rootBox.border.left + rootBox.padding.left;
  rootBox.content.y = rootBox.margin.top + rootBox.border.top + rootBox.padding.top;
  _layoutBlock(rootBox, viewportWidth, measurer);
  return rootBox;
}

/// Build the layout tree from the styled tree, skipping display:none.
LayoutBox _buildLayoutTree(StyledNode styled, String? parentHref) {
  // Track link context.
  String? href = parentHref;
  if (styled.node is Element && (styled.node as Element).tagName == 'a') {
    href = (styled.node as Element).attributes['href'];
  }

  // Text node -> text layout box.
  if (styled.node is Text) {
    final box = LayoutBox(LayoutType.text, styled);
    box.text = (styled.node as Text).data;
    box.linkHref = href;
    return box;
  }

  // <br> -> line-break marker in inline flow.
  if (styled.node is Element && (styled.node as Element).tagName == 'br') {
    final box = LayoutBox(LayoutType.text, styled);
    box.text = '\n';
    box.linkHref = href;
    return box;
  }

  final display = styled.display;
  if (display == Display.none) {
    final box = LayoutBox(LayoutType.block, styled);
    box.content = Rect(0, 0, 0, 0);
    return box;
  }

  // Image element -> inline-replaced box.
  if (styled.node is Element && (styled.node as Element).tagName == 'img') {
    final el = styled.node as Element;
    final box = LayoutBox(LayoutType.inline, styled);
    box.linkHref = href;
    box.imageUrl = el.attributes['src'] ?? '';
    final w = double.tryParse(el.attributes['width'] ?? '') ?? 0;
    final h = double.tryParse(el.attributes['height'] ?? '') ?? 0;
    box.imageWidth = w > 0 ? w : 300;
    box.imageHeight = h > 0 ? h : 150;
    _applyVisualProperties(box, styled);
    return box;
  }

  // Embedded/media elements -> inline-replaced boxes (video, audio, iframe, etc.).
  if (styled.node is Element) {
    final el = styled.node as Element;
    final tag = el.tagName;
    if (const {'video', 'audio', 'canvas', 'iframe', 'object', 'embed', 'svg'}
        .contains(tag)) {
      final box = LayoutBox(LayoutType.inline, styled);
      box.linkHref = href;
      box.imageUrl = el.attributes['src'] ?? el.attributes['data'] ?? '';
      final w = double.tryParse(el.attributes['width'] ?? '') ?? 0;
      final h = double.tryParse(el.attributes['height'] ?? '') ?? 0;
      box.imageWidth = w > 0 ? w : _defaultMediaWidth(tag);
      box.imageHeight = h > 0 ? h : _defaultMediaHeight(tag);
      _applyVisualProperties(box, styled);
      return box;
    }
  }

  // Form elements -> interactive boxes.
  if (styled.node is Element) {
    final el = styled.node as Element;
    final tag = el.tagName;
    if (const {'input', 'button', 'select', 'textarea', 'progress', 'meter'}
        .contains(tag)) {
      final box = LayoutBox(LayoutType.inline, styled);
      box.linkHref = href;
      box.formTag = tag;
      box.formType = el.attributes['type'] ?? (tag == 'button' ? 'button' : 'text');
      box.formValue = el.attributes['value'] ?? el.textContent;
      box.formPlaceholder = el.attributes['placeholder'] ?? '';
      box.formName = el.attributes['name'] ?? '';
      box.formChecked = el.attributes.containsKey('checked');

      // Find parent <form> action/method.
      Node? ancestor = el.parent;
      while (ancestor != null) {
        if (ancestor is Element && ancestor.tagName == 'form') {
          box.formAction = ancestor.attributes['action'] ?? '';
          box.formMethod = (ancestor.attributes['method'] ?? 'GET').toUpperCase();
          break;
        }
        ancestor = ancestor.parent;
      }

      // For <select>: extract <option> children.
      if (tag == 'select') {
        final options = <String>[];
        final optionValues = <String>[];
        for (final child in el.children) {
          if (child is Element && child.tagName == 'option') {
            options.add(child.textContent.trim());
            optionValues.add(child.attributes['value'] ?? child.textContent.trim());
            if (child.attributes.containsKey('selected') && box.formValue!.isEmpty) {
              box.formValue = child.textContent.trim();
            }
          }
        }
        box.formOptions = options;
        box.formOptionValues = optionValues;
        if (box.formValue!.isEmpty && options.isNotEmpty) {
          box.formValue = options.first;
        }
      }

      _applyVisualProperties(box, styled);
      return box;
    }
  }

  // Flexbox layout.
  if (display == Display.flex || display == Display.inlineFlex) {
    final box = LayoutBox(LayoutType.flex, styled);
    box.linkHref = href;
    _applyVisualProperties(box, styled);

    for (final child in styled.children) {
      if (child.display == Display.none) continue;
      final childBox = _buildLayoutTree(child, href);
      box.children.add(childBox);
    }
    return box;
  }

  // Grid layout.
  if (display == Display.grid || display == Display.inlineGrid) {
    final box = LayoutBox(LayoutType.grid, styled);
    box.linkHref = href;
    _applyVisualProperties(box, styled);

    for (final child in styled.children) {
      if (child.display == Display.none) continue;
      final childBox = _buildLayoutTree(child, href);
      box.children.add(childBox);
    }
    return box;
  }

  // Table layout.
  if (display == Display.table || display == Display.inlineTable) {
    final box = LayoutBox(LayoutType.table, styled);
    box.linkHref = href;
    _applyVisualProperties(box, styled);

    for (final child in styled.children) {
      if (child.display == Display.none) continue;
      final childBox = _buildLayoutTree(child, href);
      box.children.add(childBox);
    }
    return box;
  }

  final type = (display == Display.inline || display == Display.inlineBlock)
      ? LayoutType.inline
      : LayoutType.block;
  final box = LayoutBox(type, styled);
  box.linkHref = href;
  _applyVisualProperties(box, styled);

  for (final child in styled.children) {
    if (child.display == Display.none) continue;
    final childBox = _buildLayoutTree(child, href);
    box.children.add(childBox);
  }

  // If an inline box contains any block children, promote it to block.
  if (box.layoutType == LayoutType.inline &&
      box.children.any((c) => c.layoutType == LayoutType.block)) {
    box.layoutType = LayoutType.block;
  }

  return box;
}

/// Apply visual properties from styled node to layout box.
void _applyVisualProperties(LayoutBox box, StyledNode styled) {
  // Float and clear.
  box.float_ = styled.prop('float', 'none');
  box.clear = styled.prop('clear', 'none');

  // Position.
  box.position = styled.prop('position', 'static');
  final topStr = styled.prop('top', '');
  final rightStr = styled.prop('right', '');
  final bottomStr = styled.prop('bottom', '');
  final leftStr = styled.prop('left', '');
  if (topStr.isNotEmpty && topStr != 'auto') box.posTop = _parsePx(topStr);
  if (rightStr.isNotEmpty && rightStr != 'auto') box.posRight = _parsePx(rightStr);
  if (bottomStr.isNotEmpty && bottomStr != 'auto') box.posBottom = _parsePx(bottomStr);
  if (leftStr.isNotEmpty && leftStr != 'auto') box.posLeft = _parsePx(leftStr);

  // Z-index.
  final zi = styled.prop('z-index', '');
  if (zi.isNotEmpty && zi != 'auto') box.zIndex = int.tryParse(zi) ?? 0;

  // Overflow.
  box.overflow = styled.prop('overflow', 'visible');

  // Opacity.
  final opacityStr = styled.prop('opacity', '');
  if (opacityStr.isNotEmpty) {
    box.opacity = (double.tryParse(opacityStr) ?? 1.0).clamp(0.0, 1.0);
  }

  // Border radius.
  _applyBorderRadius(box, styled);

  // Box shadow.
  final shadow = styled.prop('box-shadow', '');
  if (shadow.isNotEmpty && shadow != 'none') box.boxShadow = shadow;
}

/// Parse border-radius from styled node.
void _applyBorderRadius(LayoutBox box, StyledNode styled) {
  final br = styled.prop('border-radius', '');
  if (br.isNotEmpty) {
    final parts = br.trim().split(RegExp(r'\s+'));
    switch (parts.length) {
      case 1:
        final r = _parsePx(parts[0]);
        box.borderRadiusTL = r;
        box.borderRadiusTR = r;
        box.borderRadiusBR = r;
        box.borderRadiusBL = r;
      case 2:
        box.borderRadiusTL = _parsePx(parts[0]);
        box.borderRadiusTR = _parsePx(parts[1]);
        box.borderRadiusBR = _parsePx(parts[0]);
        box.borderRadiusBL = _parsePx(parts[1]);
      case 3:
        box.borderRadiusTL = _parsePx(parts[0]);
        box.borderRadiusTR = _parsePx(parts[1]);
        box.borderRadiusBR = _parsePx(parts[2]);
        box.borderRadiusBL = _parsePx(parts[1]);
      case 4:
        box.borderRadiusTL = _parsePx(parts[0]);
        box.borderRadiusTR = _parsePx(parts[1]);
        box.borderRadiusBR = _parsePx(parts[2]);
        box.borderRadiusBL = _parsePx(parts[3]);
    }
  }
  // Individual corners override.
  final tlStr = styled.prop('border-top-left-radius', '');
  if (tlStr.isNotEmpty) box.borderRadiusTL = _parsePx(tlStr);
  final trStr = styled.prop('border-top-right-radius', '');
  if (trStr.isNotEmpty) box.borderRadiusTR = _parsePx(trStr);
  final brStr = styled.prop('border-bottom-right-radius', '');
  if (brStr.isNotEmpty) box.borderRadiusBR = _parsePx(brStr);
  final blStr = styled.prop('border-bottom-left-radius', '');
  if (blStr.isNotEmpty) box.borderRadiusBL = _parsePx(blStr);
}

// ── Block layout ────────────────────────────────────────────────────

void _layoutBlock(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  _computeBoxDimensions(box, containerWidth);

  // Content width = container minus our horizontal margin/border/padding.
  final contentWidth = box.content.width;

  // Lay out children.
  if (box.layoutType == LayoutType.table) {
    _layoutTable(box, contentWidth, measurer);
  } else if (box.layoutType == LayoutType.grid) {
    _layoutGrid(box, contentWidth, measurer);
  } else if (box.layoutType == LayoutType.flex) {
    _layoutFlex(box, contentWidth, measurer);
  } else if (_isTableRow(box)) {
    _layoutTableRow(box, contentWidth, measurer);
  } else if (_hasInlineChildren(box)) {
    _layoutInlineChildren(box, contentWidth, measurer);
  } else {
    _layoutBlockChildren(box, contentWidth, measurer);
  }

  // If height was not set explicitly, use content height.
  // Table, grid, and flex layouts compute their own height internally.
  final heightProp = box.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    if (box.layoutType != LayoutType.table &&
        box.layoutType != LayoutType.flex &&
        box.layoutType != LayoutType.grid) {
      double h = 0;
      for (final child in box.children) {
        // Skip absolute/fixed children from contributing to height.
        if (child.position == 'absolute' || child.position == 'fixed') continue;
        h = math.max(h, child.marginBox.y + child.marginBox.height - box.content.y);
      }
      box.content.height = h;
    }
  }

  // If this box establishes a BFC (has overflow != visible, or is the root),
  // expand height to contain all floats.
  final overflowProp = box.styledNode?.prop('overflow', 'visible') ?? 'visible';
  if (overflowProp != 'visible' || box.position == 'absolute' || box.position == 'fixed') {
    // Already enclosed by BFC rules.
  }

  // Apply min/max height constraints.
  _applyHeightConstraints(box);

  // Apply positioning offsets for relative/absolute.
  _applyPositioning(box);
}

void _computeBoxDimensions(LayoutBox box, double containerWidth) {
  final s = box.styledNode;
  if (s == null) {
    // Anonymous box: no margins/borders/padding, fills container width.
    box.content.width = math.max(0, containerWidth);
    return;
  }

  box.margin = _parseEdges(s, 'margin');
  box.padding = _parseEdges(s, 'padding');
  box.border = _parseBorderWidths(s);

  final boxSizing = s.prop('box-sizing', 'content-box');

  // Width: explicit or fill container.
  final widthStr = s.prop('width', '');
  if (widthStr.isNotEmpty && widthStr != 'auto') {
    double w = _parsePx(widthStr, containerWidth);
    if (boxSizing == 'border-box') {
      // border-box: width includes padding + border.
      w -= box.padding.left + box.padding.right + box.border.left + box.border.right;
    }
    box.content.width = math.max(0, w);

    // Handle margin: auto for horizontal centering on block elements with explicit width.
    final marginShorthand = s.prop('margin', '');
    final marginLeftStr = s.prop('margin-left', '');
    final marginRightStr = s.prop('margin-right', '');
    final hasAutoLeft = marginLeftStr == 'auto' || (marginShorthand.contains('auto') && marginLeftStr.isEmpty);
    final hasAutoRight = marginRightStr == 'auto' || (marginShorthand.contains('auto') && marginRightStr.isEmpty);
    if (hasAutoLeft && hasAutoRight) {
      final totalOuter = box.content.width + box.border.left + box.border.right +
          box.padding.left + box.padding.right;
      final autoMargin = math.max(0.0, (containerWidth - totalOuter) / 2);
      box.margin = EdgeSizes(box.margin.top, autoMargin, box.margin.bottom, autoMargin);
    } else if (hasAutoLeft) {
      final totalOuter = box.content.width + box.margin.right + box.border.left + box.border.right +
          box.padding.left + box.padding.right;
      final autoMargin = math.max(0.0, containerWidth - totalOuter);
      box.margin = EdgeSizes(box.margin.top, box.margin.right, box.margin.bottom, autoMargin);
    } else if (hasAutoRight) {
      final totalOuter = box.content.width + box.margin.left + box.border.left + box.border.right +
          box.padding.left + box.padding.right;
      final autoMargin = math.max(0.0, containerWidth - totalOuter);
      box.margin = EdgeSizes(box.margin.top, autoMargin, box.margin.bottom, box.margin.left);
    }
  } else {
    box.content.width = containerWidth -
        box.margin.left -
        box.margin.right -
        box.border.left -
        box.border.right -
        box.padding.left -
        box.padding.right;
  }
  box.content.width = math.max(0, box.content.width);

  // Apply min/max width constraints.
  final minW = s.prop('min-width', '');
  if (minW.isNotEmpty && minW != 'auto') {
    final mw = _parsePx(minW, containerWidth);
    box.content.width = math.max(box.content.width, mw);
  }
  final maxW = s.prop('max-width', '');
  if (maxW.isNotEmpty && maxW != 'none') {
    final mw = _parsePx(maxW, containerWidth);
    box.content.width = math.min(box.content.width, mw);
  }

  // Height: explicit or auto (computed later).
  final heightStr = s.prop('height', '');
  if (heightStr.isNotEmpty && heightStr != 'auto') {
    double h = _parsePx(heightStr, 0);
    if (boxSizing == 'border-box') {
      h -= box.padding.top + box.padding.bottom + box.border.top + box.border.bottom;
    }
    box.content.height = math.max(0, h);
  }
}

/// Apply min/max height constraints after layout.
void _applyHeightConstraints(LayoutBox box) {
  final s = box.styledNode;
  if (s == null) return;

  final minH = s.prop('min-height', '');
  if (minH.isNotEmpty && minH != 'auto') {
    final mh = _parsePx(minH, 0);
    box.content.height = math.max(box.content.height, mh);
  }
  final maxH = s.prop('max-height', '');
  if (maxH.isNotEmpty && maxH != 'none') {
    final mh = _parsePx(maxH, 0);
    box.content.height = math.min(box.content.height, mh);
  }
}

/// Apply CSS positioning offsets for relative, absolute, and fixed.
void _applyPositioning(LayoutBox box) {
  if (box.position == 'relative') {
    if (box.posTop != null) box.content.y += box.posTop!;
    if (box.posLeft != null) box.content.x += box.posLeft!;
    if (box.posBottom != null && box.posTop == null) box.content.y -= box.posBottom!;
    if (box.posRight != null && box.posLeft == null) box.content.x -= box.posRight!;
  }
}

// ── Float tracking ──────────────────────────────────────────────────

class _FloatRect {
  final double x, y, width, height;
  final String side; // 'left' or 'right'
  _FloatRect(this.x, this.y, this.width, this.height, this.side);
  double get bottom => y + height;
  double get right_ => x + width;
}

class _FloatContext {
  final List<_FloatRect> lefts = [];
  final List<_FloatRect> rights = [];

  void addFloat(_FloatRect f) {
    if (f.side == 'left') lefts.add(f);
    else rights.add(f);
  }

  /// Get the Y position needed to clear past floats.
  double clearY(String clear, double currentY) {
    double y = currentY;
    if (clear == 'left' || clear == 'both') {
      for (final f in lefts) {
        if (f.bottom > y) y = f.bottom;
      }
    }
    if (clear == 'right' || clear == 'both') {
      for (final f in rights) {
        if (f.bottom > y) y = f.bottom;
      }
    }
    return y;
  }

  /// Get available width and left offset at a given Y, accounting for floats.
  /// Returns (leftEdge, availableWidth).
  (double, double) availableAt(double y, double height, double containerX, double containerWidth) {
    double leftEdge = containerX;
    double rightEdge = containerX + containerWidth;

    for (final f in lefts) {
      if (y < f.bottom && y + height > f.y) {
        leftEdge = math.max(leftEdge, f.right_);
      }
    }
    for (final f in rights) {
      if (y < f.bottom && y + height > f.y) {
        rightEdge = math.min(rightEdge, f.x);
      }
    }

    return (leftEdge, math.max(0, rightEdge - leftEdge));
  }

  /// Find a Y position where the given width fits.
  double findYForWidth(double startY, double neededWidth, double height, double containerX, double containerWidth) {
    double y = startY;
    for (int i = 0; i < 100; i++) { // Safety limit
      final (_, avail) = availableAt(y, height, containerX, containerWidth);
      if (avail >= neededWidth || avail >= containerWidth) return y;
      // Move past the lowest float that's blocking us
      double nextY = double.infinity;
      for (final f in [...lefts, ...rights]) {
        if (f.bottom > y && f.bottom < nextY) nextY = f.bottom;
      }
      if (nextY == double.infinity) return y;
      y = nextY;
    }
    return y;
  }
}

// ── Block children layout ───────────────────────────────────────────

void _layoutBlockChildren(
  LayoutBox box,
  double containerWidth,
  TextMeasurer measurer, [
  _FloatContext? parentFloats,
]) {
  _ensureBlockChildren(box);

  final floats = parentFloats ?? _FloatContext();
  double cursorY = box.content.y;
  double prevMarginBottom = 0;

  for (final child in box.children) {
    try {
      // Skip absolute/fixed positioned children from normal flow.
      if (child.position == 'absolute' || child.position == 'fixed') {
        _layoutAbsoluteChild(child, box, containerWidth, measurer);
        continue;
      }

      _computeBoxDimensions(child, containerWidth);

      // Handle clear property.
      if (child.clear != 'none') {
        cursorY = floats.clearY(child.clear, cursorY);
      }

      // Handle floated children.
      if (child.float_ == 'left' || child.float_ == 'right') {
        _layoutFloatedChild(child, box, containerWidth, cursorY, floats, measurer);
        continue;
      }

      // Collapse adjacent vertical margins.
      final collapsed = math.max(prevMarginBottom, child.margin.top);

      if (child.layoutType == LayoutType.text ||
          child.layoutType == LayoutType.inline) {
        // Get available space accounting for floats.
        final childTop = cursorY + collapsed + child.border.top + child.padding.top;
        final (leftEdge, availWidth) = floats.availableAt(
            childTop, 20, box.content.x, containerWidth);
        child.content.x = leftEdge + child.margin.left + child.border.left + child.padding.left;
        child.content.y = childTop;
        child.content.width = math.max(0, availWidth -
            child.margin.left - child.margin.right -
            child.border.left - child.border.right -
            child.padding.left - child.padding.right);
        _layoutInlineContent(child, child.content.width, measurer);
      } else {
        // Block child — shrink around floats.
        final childTop = cursorY + collapsed + child.border.top + child.padding.top;
        final (leftEdge, availWidth) = floats.availableAt(
            childTop, 20, box.content.x, containerWidth);
        child.content.x = leftEdge +
            child.margin.left +
            child.border.left +
            child.padding.left;
        child.content.y = childTop;
        // If the block has no explicit width, constrain to available space.
        final explicitWidth = child.styledNode?.prop('width', '') ?? '';
        if (explicitWidth.isEmpty || explicitWidth == 'auto') {
          child.content.width = math.max(0, availWidth -
              child.margin.left - child.margin.right -
              child.border.left - child.border.right -
              child.padding.left - child.padding.right);
        }
        _layoutBlock(child, child.content.width, measurer);
      }

      cursorY = child.content.y + child.content.height +
          child.padding.bottom + child.border.bottom;
      prevMarginBottom = child.margin.bottom;
    } catch (_) {
      // Skip this child on error; continue laying out remaining content.
    }
  }
}

/// Layout a floated child and register it in the float context.
void _layoutFloatedChild(
  LayoutBox child,
  LayoutBox containingBlock,
  double containerWidth,
  double cursorY,
  _FloatContext floats,
  TextMeasurer measurer,
) {
  // Compute the child's box model.
  _computeBoxDimensions(child, containerWidth);

  // Determine the child's total outer width.
  final outerWidth = child.content.width +
      child.margin.left + child.margin.right +
      child.border.left + child.border.right +
      child.padding.left + child.padding.right;
  final outerHeight = 20.0; // Estimate; will refine after layout.

  // Find a Y position where the float fits.
  final y = floats.findYForWidth(
      cursorY, outerWidth, outerHeight,
      containingBlock.content.x, containerWidth);

  final (leftEdge, availWidth) = floats.availableAt(
      y, outerHeight, containingBlock.content.x, containerWidth);

  if (child.float_ == 'left') {
    child.content.x = leftEdge + child.margin.left + child.border.left + child.padding.left;
  } else {
    // Right float: align to the right edge.
    final rightEdge = leftEdge + availWidth;
    child.content.x = rightEdge - outerWidth +
        child.margin.left + child.border.left + child.padding.left;
  }

  child.content.y = y + child.margin.top + child.border.top + child.padding.top;

  // Layout child content.
  if (child.layoutType == LayoutType.flex) {
    _layoutFlex(child, child.content.width, measurer);
  } else if (child.layoutType == LayoutType.table) {
    _layoutTable(child, child.content.width, measurer);
  } else if (_hasInlineChildren(child)) {
    _layoutInlineChildren(child, child.content.width, measurer);
  } else {
    _layoutBlockChildren(child, child.content.width, measurer, floats);
  }

  // Auto-height.
  final heightProp = child.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    double h = 0;
    for (final c in child.children) {
      h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
    }
    child.content.height = math.max(h, child.content.height);
  }
  _applyHeightConstraints(child);

  // Register this float.
  final totalHeight = child.content.height +
      child.margin.top + child.margin.bottom +
      child.border.top + child.border.bottom +
      child.padding.top + child.padding.bottom;

  floats.addFloat(_FloatRect(
    child.content.x - child.border.left - child.padding.left - child.margin.left,
    y,
    outerWidth,
    totalHeight,
    child.float_,
  ));

}

/// Layout an absolutely positioned child within a containing block.
void _layoutAbsoluteChild(
  LayoutBox child,
  LayoutBox containingBlock,
  double containerWidth,
  TextMeasurer measurer,
) {
  _computeBoxDimensions(child, containerWidth);

  // Position relative to containing block.
  final cbx = containingBlock.content.x;
  final cby = containingBlock.content.y;
  final cbw = containingBlock.content.width;
  final cbh = containingBlock.content.height;

  if (child.posLeft != null) {
    child.content.x = cbx + child.posLeft! + child.margin.left + child.border.left + child.padding.left;
  } else if (child.posRight != null) {
    child.content.x = cbx + cbw - child.posRight! - child.content.width - child.margin.right - child.border.right - child.padding.right;
  } else {
    child.content.x = cbx + child.margin.left + child.border.left + child.padding.left;
  }

  if (child.posTop != null) {
    child.content.y = cby + child.posTop! + child.margin.top + child.border.top + child.padding.top;
  } else if (child.posBottom != null) {
    child.content.y = cby + cbh - child.posBottom! - child.content.height - child.margin.bottom - child.border.bottom - child.padding.bottom;
  } else {
    child.content.y = cby + child.margin.top + child.border.top + child.padding.top;
  }

  // Layout child content.
  if (child.layoutType == LayoutType.flex) {
    _layoutFlex(child, child.content.width, measurer);
  } else if (child.layoutType == LayoutType.table) {
    _layoutTable(child, child.content.width, measurer);
  } else if (_hasInlineChildren(child)) {
    _layoutInlineChildren(child, child.content.width, measurer);
  } else {
    _layoutBlockChildren(child, child.content.width, measurer);
  }

  // Auto-height.
  final heightProp = child.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    double h = 0;
    for (final c in child.children) {
      h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
    }
    child.content.height = h;
  }
  _applyHeightConstraints(child);
  _applyPositioning(child);
}

/// Wrap consecutive inline/text children in anonymous block boxes.
void _ensureBlockChildren(LayoutBox box) {
  if (box.children.isEmpty) return;

  final hasBlock = box.children.any((c) =>
      c.layoutType == LayoutType.block || c.layoutType == LayoutType.anonymous ||
      c.layoutType == LayoutType.flex || c.layoutType == LayoutType.grid ||
      c.layoutType == LayoutType.table);
  final hasInline = box.children.any((c) =>
      c.layoutType == LayoutType.inline || c.layoutType == LayoutType.text);

  if (!hasBlock || !hasInline) return;

  final newChildren = <LayoutBox>[];
  List<LayoutBox>? inlineRun;

  for (final child in box.children) {
    if (child.layoutType == LayoutType.block ||
        child.layoutType == LayoutType.anonymous ||
        child.layoutType == LayoutType.flex ||
        child.layoutType == LayoutType.grid ||
        child.layoutType == LayoutType.table) {
      if (inlineRun != null) {
        final anon = LayoutBox(LayoutType.anonymous);
        anon.children.addAll(inlineRun);
        newChildren.add(anon);
        inlineRun = null;
      }
      newChildren.add(child);
    } else {
      inlineRun ??= [];
      inlineRun.add(child);
    }
  }

  if (inlineRun != null) {
    final anon = LayoutBox(LayoutType.anonymous);
    anon.children.addAll(inlineRun);
    newChildren.add(anon);
  }

  box.children.clear();
  box.children.addAll(newChildren);
}

// ── Flexbox layout ──────────────────────────────────────────────────

void _layoutFlex(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  final s = box.styledNode;
  final direction = s?.prop('flex-direction', 'row') ?? 'row';
  final justifyContent = s?.prop('justify-content', 'flex-start') ?? 'flex-start';
  final alignItems = s?.prop('align-items', 'stretch') ?? 'stretch';
  final flexWrap = s?.prop('flex-wrap', 'nowrap') ?? 'nowrap';
  final gap = _parsePx(s?.prop('gap', '0') ?? '0');

  final isRow = direction == 'row' || direction == 'row-reverse';
  final isReverse = direction == 'row-reverse' || direction == 'column-reverse';

  // Compute child sizes.
  final flexChildren = <LayoutBox>[];
  for (final child in box.children) {
    if (child.position == 'absolute' || child.position == 'fixed') {
      _layoutAbsoluteChild(child, box, containerWidth, measurer);
      continue;
    }
    _computeBoxDimensions(child, isRow ? containerWidth : box.content.width);
    flexChildren.add(child);
  }

  if (flexChildren.isEmpty) return;

  if (isRow) {
    _layoutFlexRow(box, flexChildren, containerWidth, justifyContent, alignItems,
        flexWrap, gap, isReverse, measurer);
  } else {
    _layoutFlexColumn(box, flexChildren, containerWidth, justifyContent, alignItems,
        flexWrap, gap, isReverse, measurer);
  }
}

void _layoutFlexRow(
  LayoutBox box,
  List<LayoutBox> children,
  double containerWidth,
  String justifyContent,
  String alignItems,
  String flexWrap,
  double gap,
  bool isReverse,
  TextMeasurer measurer,
) {
  // Measure each child's natural width.
  final childWidths = <double>[];
  final childFlexGrow = <double>[];
  double totalFixedWidth = 0;

  for (final child in children) {
    final flexGrow = double.tryParse(child.styledNode?.prop('flex-grow', '0') ?? '0') ?? 0;
    final flexStr = child.styledNode?.prop('flex', '') ?? '';
    double grow = flexGrow;
    if (flexStr.isNotEmpty && flexStr != 'none') {
      final parts = flexStr.split(RegExp(r'\s+'));
      grow = double.tryParse(parts[0]) ?? 0;
    }
    childFlexGrow.add(grow);

    // Natural width of this child.
    double w = child.content.width + child.margin.left + child.margin.right +
        child.border.left + child.border.right + child.padding.left + child.padding.right;
    childWidths.add(w);
    if (grow == 0) totalFixedWidth += w;
  }

  // Distribute remaining space to flex-grow items.
  final totalGrow = childFlexGrow.fold(0.0, (sum, g) => sum + g);
  double totalGaps = gap * (children.length - 1);
  double availableSpace = box.content.width - totalFixedWidth - totalGaps;
  if (totalGrow > 0 && availableSpace > 0) {
    for (int i = 0; i < children.length; i++) {
      if (childFlexGrow[i] > 0) {
        final share = availableSpace * (childFlexGrow[i] / totalGrow);
        childWidths[i] = share;
        children[i].content.width = math.max(0, share -
            children[i].margin.left - children[i].margin.right -
            children[i].border.left - children[i].border.right -
            children[i].padding.left - children[i].padding.right);
      }
    }
  }

  // Layout each child and determine heights.
  double maxChildHeight = 0;
  for (int i = 0; i < children.length; i++) {
    final child = children[i];
    // Layout child content.
    child.content.x = 0; // Will be set below.
    child.content.y = 0;
    if (child.layoutType == LayoutType.flex) {
      _layoutFlex(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.table) {
      _layoutTable(child, child.content.width, measurer);
    } else if (_hasInlineChildren(child)) {
      _layoutInlineContent(child, child.content.width, measurer);
    } else {
      _layoutBlockChildren(child, child.content.width, measurer);
    }
    // Auto-height.
    final heightProp = child.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final c in child.children) {
        h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
      }
      child.content.height = h;
    }
    _applyHeightConstraints(child);
    maxChildHeight = math.max(maxChildHeight,
        child.content.height + child.margin.top + child.margin.bottom +
        child.border.top + child.border.bottom + child.padding.top + child.padding.bottom);
  }

  // Calculate total used width.
  double totalUsedWidth = 0;
  for (final w in childWidths) totalUsedWidth += w;
  totalUsedWidth += totalGaps;

  // Justify content: determine starting X and spacing.
  double startX = box.content.x;
  double extraSpacing = 0;
  final freeSpace = box.content.width - totalUsedWidth;

  switch (justifyContent) {
    case 'center':
      startX += freeSpace / 2;
    case 'flex-end':
    case 'end':
      startX += freeSpace;
    case 'space-between':
      if (children.length > 1) {
        extraSpacing = freeSpace / (children.length - 1);
      }
    case 'space-around':
      if (children.isNotEmpty) {
        final space = freeSpace / children.length;
        startX += space / 2;
        extraSpacing = space;
      }
    case 'space-evenly':
      if (children.isNotEmpty) {
        final space = freeSpace / (children.length + 1);
        startX += space;
        extraSpacing = space;
      }
  }

  // Position children.
  double cursorX = startX;
  final ordered = isReverse ? children.reversed.toList() : children;
  for (int i = 0; i < ordered.length; i++) {
    final child = ordered[i];
    child.content.x = cursorX + child.margin.left + child.border.left + child.padding.left;
    final totalChildHeight = child.content.height + child.margin.top + child.margin.bottom +
        child.border.top + child.border.bottom + child.padding.top + child.padding.bottom;

    switch (alignItems) {
      case 'center':
        child.content.y = box.content.y + (maxChildHeight - totalChildHeight) / 2 +
            child.margin.top + child.border.top + child.padding.top;
      case 'flex-end':
      case 'end':
        child.content.y = box.content.y + maxChildHeight - totalChildHeight +
            child.margin.top + child.border.top + child.padding.top;
      case 'stretch':
        child.content.y = box.content.y + child.margin.top + child.border.top + child.padding.top;
        final heightProp = child.styledNode?.prop('height', '') ?? '';
        if (heightProp.isEmpty || heightProp == 'auto') {
          child.content.height = maxChildHeight -
              child.margin.top - child.margin.bottom -
              child.border.top - child.border.bottom -
              child.padding.top - child.padding.bottom;
        }
      default: // flex-start / start / baseline
        child.content.y = box.content.y + child.margin.top + child.border.top + child.padding.top;
    }

    // Re-layout children with correct positions.
    _relayoutChildPositions(child);

    cursorX += childWidths[isReverse ? (ordered.length - 1 - i) : i] + gap + extraSpacing;
  }

  // Set box height.
  final heightProp = box.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    box.content.height = maxChildHeight;
  }
}

void _layoutFlexColumn(
  LayoutBox box,
  List<LayoutBox> children,
  double containerWidth,
  String justifyContent,
  String alignItems,
  String flexWrap,
  double gap,
  bool isReverse,
  TextMeasurer measurer,
) {
  // Layout each child at full width first to get heights.
  final childHeights = <double>[];
  final childFlexGrow = <double>[];

  for (final child in children) {
    final flexGrow = double.tryParse(child.styledNode?.prop('flex-grow', '0') ?? '0') ?? 0;
    final flexStr = child.styledNode?.prop('flex', '') ?? '';
    double grow = flexGrow;
    if (flexStr.isNotEmpty && flexStr != 'none') {
      final parts = flexStr.split(RegExp(r'\s+'));
      grow = double.tryParse(parts[0]) ?? 0;
    }
    childFlexGrow.add(grow);

    child.content.x = 0;
    child.content.y = 0;
    if (child.layoutType == LayoutType.flex) {
      _layoutFlex(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.table) {
      _layoutTable(child, child.content.width, measurer);
    } else if (_hasInlineChildren(child)) {
      _layoutInlineContent(child, child.content.width, measurer);
    } else {
      _layoutBlockChildren(child, child.content.width, measurer);
    }

    final heightProp = child.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final c in child.children) {
        h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
      }
      child.content.height = h;
    }
    _applyHeightConstraints(child);

    final totalH = child.content.height + child.margin.top + child.margin.bottom +
        child.border.top + child.border.bottom + child.padding.top + child.padding.bottom;
    childHeights.add(totalH);
  }

  // Calculate total height.
  double totalHeight = 0;
  for (final h in childHeights) totalHeight += h;
  totalHeight += gap * (children.length - 1);

  // Justify content.
  double startY = box.content.y;
  double extraSpacing = 0;
  final freeSpace = box.content.height - totalHeight;

  if (freeSpace > 0) {
    switch (justifyContent) {
      case 'center':
        startY += freeSpace / 2;
      case 'flex-end':
      case 'end':
        startY += freeSpace;
      case 'space-between':
        if (children.length > 1) {
          extraSpacing = freeSpace / (children.length - 1);
        }
      case 'space-around':
        if (children.isNotEmpty) {
          final space = freeSpace / children.length;
          startY += space / 2;
          extraSpacing = space;
        }
      case 'space-evenly':
        if (children.isNotEmpty) {
          final space = freeSpace / (children.length + 1);
          startY += space;
          extraSpacing = space;
        }
    }
  }

  // Position children.
  double cursorY = startY;
  final ordered = isReverse ? children.reversed.toList() : children;
  for (int i = 0; i < ordered.length; i++) {
    final child = ordered[i];
    child.content.y = cursorY + child.margin.top + child.border.top + child.padding.top;
    final totalChildWidth = child.content.width + child.margin.left + child.margin.right +
        child.border.left + child.border.right + child.padding.left + child.padding.right;

    switch (alignItems) {
      case 'center':
        child.content.x = box.content.x + (box.content.width - totalChildWidth) / 2 +
            child.margin.left + child.border.left + child.padding.left;
      case 'flex-end':
      case 'end':
        child.content.x = box.content.x + box.content.width - totalChildWidth +
            child.margin.left + child.border.left + child.padding.left;
      case 'stretch':
        child.content.x = box.content.x + child.margin.left + child.border.left + child.padding.left;
        child.content.width = box.content.width -
            child.margin.left - child.margin.right -
            child.border.left - child.border.right -
            child.padding.left - child.padding.right;
      default: // flex-start / start
        child.content.x = box.content.x + child.margin.left + child.border.left + child.padding.left;
    }

    _relayoutChildPositions(child);

    cursorY += childHeights[isReverse ? (ordered.length - 1 - i) : i] + gap + extraSpacing;
  }

  // Set box height.
  final heightProp = box.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    box.content.height = cursorY - box.content.y;
  }
}

/// After flex positioning, offset all sub-children so they are positioned
/// relative to the parent's final content origin.
void _relayoutChildPositions(LayoutBox box) {
  if (box.children.isEmpty) return;
  // Offset all descendant boxes by the difference between the parent's
  // final position and origin (0,0) where they were originally laid out.
  final dx = box.content.x;
  final dy = box.content.y;
  if (dx == 0 && dy == 0) return;
  for (final child in box.children) {
    _offsetBoxTree(child, dx, dy);
  }
}

/// Recursively offset a box and all its descendants.
void _offsetBoxTree(LayoutBox box, double dx, double dy) {
  box.content.x += dx;
  box.content.y += dy;
  for (final child in box.children) {
    _offsetBoxTree(child, dx, dy);
  }
}

// ── Grid layout ─────────────────────────────────────────────────────

void _layoutGrid(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  final s = box.styledNode;
  final gap = _parsePx(s?.prop('gap', '0') ?? '0');
  final rowGap = _parsePx(s?.prop('row-gap', '') ?? '', gap);
  final colGap = _parsePx(s?.prop('column-gap', '') ?? '', gap);

  // Parse grid-template-columns.
  final colTemplate = s?.prop('grid-template-columns', '') ?? '';
  final rowTemplate = s?.prop('grid-template-rows', '') ?? '';

  // Collect non-absolute children.
  final gridChildren = <LayoutBox>[];
  for (final child in box.children) {
    if (child.position == 'absolute' || child.position == 'fixed') {
      _layoutAbsoluteChild(child, box, containerWidth, measurer);
      continue;
    }
    gridChildren.add(child);
  }

  if (gridChildren.isEmpty) return;

  // Parse column definitions.
  final colDefs = _parseGridTemplate(colTemplate, containerWidth, colGap);
  // Auto-compute column count from children if no template.
  final numCols = colDefs.isNotEmpty ? colDefs.length : _autoGridCols(gridChildren.length, containerWidth);
  final numRows = (gridChildren.length / numCols).ceil();

  // Compute column widths.
  List<double> colWidths;
  if (colDefs.isNotEmpty) {
    // Distribute fr units in remaining space.
    colWidths = _resolveGridTracks(colDefs, containerWidth, colGap);
  } else {
    // Equal-width auto columns.
    final w = (containerWidth - colGap * (numCols - 1)) / numCols;
    colWidths = List.filled(numCols, math.max(0, w));
  }

  // Parse row heights (if provided).
  final rowDefs = _parseGridTemplate(rowTemplate, 0, rowGap);

  // First pass: layout children to determine row heights.
  final rowHeights = List.filled(numRows, 0.0);
  for (int i = 0; i < gridChildren.length; i++) {
    final child = gridChildren[i];
    final col = i % numCols;
    final row = i ~/ numCols;
    final cellWidth = colWidths[col];

    _computeBoxDimensions(child, cellWidth);
    child.content.width = math.max(0, cellWidth -
        child.margin.left - child.margin.right -
        child.border.left - child.border.right -
        child.padding.left - child.padding.right);

    child.content.x = 0;
    child.content.y = 0;
    if (child.layoutType == LayoutType.flex) {
      _layoutFlex(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.grid) {
      _layoutGrid(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.table) {
      _layoutTable(child, child.content.width, measurer);
    } else if (_hasInlineChildren(child)) {
      _layoutInlineContent(child, child.content.width, measurer);
    } else {
      _layoutBlockChildren(child, child.content.width, measurer);
    }

    // Auto-height.
    final heightProp = child.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final c in child.children) {
        h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
      }
      child.content.height = h;
    }
    _applyHeightConstraints(child);

    final totalH = child.content.height +
        child.margin.top + child.margin.bottom +
        child.border.top + child.border.bottom +
        child.padding.top + child.padding.bottom;

    // Use row template height if specified, otherwise take max of children.
    if (row < rowDefs.length && rowDefs[row].unit != 'auto') {
      rowHeights[row] = math.max(rowHeights[row], _resolveGridTracks(
          [rowDefs[row]], totalH, 0).first);
    } else {
      rowHeights[row] = math.max(rowHeights[row], totalH);
    }
  }

  // Second pass: position children.
  for (int i = 0; i < gridChildren.length; i++) {
    final child = gridChildren[i];
    final col = i % numCols;
    final row = i ~/ numCols;

    // Calculate X offset.
    double x = box.content.x;
    for (int c = 0; c < col; c++) {
      x += colWidths[c] + colGap;
    }

    // Calculate Y offset.
    double y = box.content.y;
    for (int r = 0; r < row; r++) {
      y += rowHeights[r] + rowGap;
    }

    final cellWidth = colWidths[col];
    child.content.width = math.max(0, cellWidth -
        child.margin.left - child.margin.right -
        child.border.left - child.border.right -
        child.padding.left - child.padding.right);

    child.content.x = x + child.margin.left + child.border.left + child.padding.left;
    child.content.y = y + child.margin.top + child.border.top + child.padding.top;

    // Re-layout with final position.
    if (child.layoutType == LayoutType.flex) {
      _layoutFlex(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.grid) {
      _layoutGrid(child, child.content.width, measurer);
    } else if (child.layoutType == LayoutType.table) {
      _layoutTable(child, child.content.width, measurer);
    } else if (_hasInlineChildren(child)) {
      _layoutInlineContent(child, child.content.width, measurer);
    } else {
      _layoutBlockChildren(child, child.content.width, measurer);
    }

    // Final height.
    final heightProp = child.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final c in child.children) {
        h = math.max(h, c.marginBox.y + c.marginBox.height - child.content.y);
      }
      child.content.height = h;
    }
    _applyHeightConstraints(child);
    _applyPositioning(child);
  }

  // Set box height.
  final heightProp = box.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    double totalH = 0;
    for (int r = 0; r < numRows; r++) {
      totalH += rowHeights[r];
      if (r < numRows - 1) totalH += rowGap;
    }
    box.content.height = totalH;
  }
}

/// A grid track definition: fixed px, fraction (fr), percentage, or auto.
class _GridTrack {
  final double value;
  final String unit; // 'px', 'fr', '%', 'auto', 'min-content', 'max-content'
  _GridTrack(this.value, this.unit);
}

/// Parse a grid-template-columns/rows value like "1fr 200px auto 2fr".
List<_GridTrack> _parseGridTemplate(String template, double containerSize, double gap) {
  if (template.isEmpty) return [];
  final tracks = <_GridTrack>[];

  // Handle repeat(N, ...).
  final expanded = _expandGridRepeat(template);

  for (final part in expanded.trim().split(RegExp(r'\s+'))) {
    final p = part.trim();
    if (p.isEmpty) continue;
    if (p.endsWith('fr')) {
      final n = double.tryParse(p.replaceAll('fr', '')) ?? 1;
      tracks.add(_GridTrack(n, 'fr'));
    } else if (p == 'auto' || p == 'min-content' || p == 'max-content') {
      tracks.add(_GridTrack(0, 'auto'));
    } else if (p.endsWith('%')) {
      final n = double.tryParse(p.replaceAll('%', '')) ?? 0;
      tracks.add(_GridTrack(n / 100 * containerSize, 'px'));
    } else {
      tracks.add(_GridTrack(_parsePx(p), 'px'));
    }
  }
  return tracks;
}

/// Expand repeat(N, pattern) in grid template.
String _expandGridRepeat(String template) {
  final repeatRegex = RegExp(r'repeat\(\s*(\d+)\s*,\s*([^)]+)\)');
  return template.replaceAllMapped(repeatRegex, (m) {
    final count = int.tryParse(m.group(1)!) ?? 1;
    final pattern = m.group(2)!.trim();
    return List.filled(count, pattern).join(' ');
  });
}

/// Resolve grid tracks: distribute fr units in remaining space.
List<double> _resolveGridTracks(List<_GridTrack> tracks, double totalSize, double gap) {
  final widths = List.filled(tracks.length, 0.0);
  double usedSpace = gap * (tracks.length - 1).clamp(0, double.infinity);
  double totalFr = 0;

  for (int i = 0; i < tracks.length; i++) {
    if (tracks[i].unit == 'px') {
      widths[i] = tracks[i].value;
      usedSpace += tracks[i].value;
    } else if (tracks[i].unit == 'fr') {
      totalFr += tracks[i].value;
    } else {
      // auto: will get a share of remaining space like 1fr.
      totalFr += 1;
      tracks[i] = _GridTrack(1, 'fr');
    }
  }

  final remaining = math.max(0, totalSize - usedSpace);
  if (totalFr > 0) {
    for (int i = 0; i < tracks.length; i++) {
      if (tracks[i].unit == 'fr') {
        widths[i] = remaining * (tracks[i].value / totalFr);
      }
    }
  }

  return widths;
}

/// Determine a reasonable auto column count for grids without explicit columns.
int _autoGridCols(int childCount, double containerWidth) {
  // Heuristic: aim for cells around 200-300px wide.
  final cols = (containerWidth / 250).floor().clamp(1, childCount);
  return cols;
}

// ── Inline layout ───────────────────────────────────────────────────

bool _hasInlineChildren(LayoutBox box) {
  if (box.children.isEmpty) return false;
  return box.children.every(
    (c) => c.layoutType == LayoutType.inline || c.layoutType == LayoutType.text,
  );
}

void _layoutInlineChildren(
  LayoutBox box,
  double containerWidth,
  TextMeasurer measurer,
) {
  _layoutInlineContent(box, containerWidth, measurer);
}

void _layoutInlineContent(
  LayoutBox box,
  double containerWidth,
  TextMeasurer measurer,
) {
  // Collect all inline items (text runs + replaced elements).
  final items = <_InlineItem>[];
  _collectInlineItems(box, items);

  // Clear original children.
  box.children.clear();

  // If this box IS a text node with no collected children, measure its own text.
  if (items.isEmpty && box.text != null && box.text!.isNotEmpty) {
    final s = box.styledNode;
    final fontSize = _parsePx(s?.prop('font-size', '16px') ?? '16px', 16);
    final metrics = measurer.measureText(
      box.text!,
      fontSize: fontSize,
      fontFamily: s?.prop('font-family', 'serif') ?? 'serif',
      fontWeight: s?.prop('font-weight', 'normal') ?? 'normal',
      fontStyle: s?.prop('font-style', 'normal') ?? 'normal',
      maxWidth: containerWidth,
    );
    box.textLines = metrics.lines;
    box.content.height = metrics.height;
    if (box.content.width <= 0) box.content.width = metrics.width;
    return;
  }

  if (items.isEmpty) return;

  double cursorX = box.content.x;
  double cursorY = box.content.y;
  double lineHeight = 0;

  for (final item in items) {
    if (item.isLineBreak) {
      cursorX = box.content.x;
      cursorY += lineHeight > 0 ? lineHeight : 22.4;
      lineHeight = 0;
      continue;
    }
    if (item.replacedBox != null) {
      final rb = item.replacedBox!;
      final w = rb.imageUrl != null
          ? math.min(rb.imageWidth, containerWidth)
          : _formBoxWidth(rb);
      final h = rb.imageUrl != null
          ? (rb.imageWidth > 0 ? rb.imageHeight * (w / rb.imageWidth) : rb.imageHeight)
          : _formBoxHeight(rb);

      if (cursorX + w > box.content.x + containerWidth && cursorX > box.content.x) {
        cursorX = box.content.x;
        cursorY += lineHeight;
        lineHeight = 0;
      }

      rb.content = Rect(cursorX, cursorY, w, h);
      box.children.add(rb);
      cursorX += w;
      lineHeight = math.max(lineHeight, h);
    } else if (item.textRun != null) {
      final run = item.textRun!;
      final fontSize = _parsePx(run.fontSize, 16);
      final metrics = measurer.measureText(
        run.text,
        fontSize: fontSize,
        fontFamily: run.fontFamily,
        fontWeight: run.fontWeight,
        fontStyle: run.fontStyle,
        maxWidth: containerWidth,
      );

      for (final line in metrics.lines) {
        if (cursorX + line.width > box.content.x + containerWidth &&
            cursorX > box.content.x) {
          cursorX = box.content.x;
          cursorY += lineHeight;
          lineHeight = 0;
        }

        final textBox = LayoutBox(LayoutType.text, run.styledNode);
        textBox.text = line.text;
        textBox.textLines = [line];
        textBox.linkHref = run.linkHref;
        textBox.content = Rect(cursorX, cursorY, line.width, line.height);
        box.children.add(textBox);

        cursorX += line.width;
        lineHeight = math.max(lineHeight, line.height);
      }
    }
  }

  // Only set height if not explicitly specified.
  final explicitHeight = box.styledNode?.prop('height', '') ?? '';
  if (explicitHeight.isEmpty || explicitHeight == 'auto') {
    box.content.height = (cursorY - box.content.y) + lineHeight;
  }

  // Apply text-align: shift each line of text boxes.
  final textAlign = box.styledNode?.prop('text-align', '') ?? '';
  if (textAlign == 'center' || textAlign == 'right' || textAlign == 'end') {
    _applyTextAlign(box, containerWidth, textAlign);
  }
}

/// Shift text boxes within a line to implement text-align: center/right.
void _applyTextAlign(LayoutBox box, double containerWidth, String align) {
  if (box.children.isEmpty) return;

  // Group children into lines by their Y position.
  final lines = <double, List<LayoutBox>>{};
  for (final child in box.children) {
    final y = child.content.y;
    lines.putIfAbsent(y, () => []).add(child);
  }

  for (final lineChildren in lines.values) {
    if (lineChildren.isEmpty) continue;
    // Find the rightmost edge of the line.
    double lineRight = 0;
    for (final child in lineChildren) {
      lineRight = math.max(lineRight, child.content.x + child.content.width);
    }
    final lineWidth = lineRight - box.content.x;
    final freeSpace = containerWidth - lineWidth;
    if (freeSpace <= 0) continue;

    final shift = (align == 'center') ? freeSpace / 2 : freeSpace;
    for (final child in lineChildren) {
      child.content.x += shift;
    }
  }
}

double _formBoxWidth(LayoutBox box) {
  if (box.formTag == 'textarea') return 200;
  if (box.formTag == 'select') return 150;
  if (box.formTag == 'button') return 80;
  if (box.formTag == 'progress') return 160;
  if (box.formTag == 'meter') return 80;
  if (box.formType == 'checkbox' || box.formType == 'radio') return 16;
  if (box.formType == 'range') return 160;
  if (box.formType == 'color') return 44;
  return 170; // text input default
}

double _formBoxHeight(LayoutBox box) {
  if (box.formTag == 'textarea') return 60;
  if (box.formTag == 'progress') return 16;
  if (box.formTag == 'meter') return 16;
  if (box.formType == 'checkbox' || box.formType == 'radio') return 16;
  if (box.formType == 'color') return 24;
  return 24;
}

double _defaultMediaWidth(String tag) {
  if (tag == 'audio') return 300;
  return 300;
}

double _defaultMediaHeight(String tag) {
  if (tag == 'audio') return 32;
  return 150;
}

class _InlineItem {
  final _TextRun? textRun;
  final LayoutBox? replacedBox;
  final bool isLineBreak;
  _InlineItem({this.textRun, this.replacedBox, this.isLineBreak = false});
}

void _collectInlineItems(LayoutBox box, List<_InlineItem> items) {
  for (final child in box.children) {
    if (child.imageUrl != null || child.formTag != null) {
      items.add(_InlineItem(replacedBox: child));
    } else if (child.text != null && child.text!.isNotEmpty) {
      if (child.text == '\n') {
        items.add(_InlineItem(isLineBreak: true));
      } else {
        final s = child.styledNode;
        items.add(_InlineItem(textRun: _TextRun(
          text: child.text!,
          fontSize: s?.prop('font-size', '16px') ?? '16px',
          fontFamily: s?.prop('font-family', 'serif') ?? 'serif',
          fontWeight: s?.prop('font-weight', 'normal') ?? 'normal',
          fontStyle: s?.prop('font-style', 'normal') ?? 'normal',
          styledNode: s,
          linkHref: child.linkHref,
        )));
      }
    } else if (child.children.isNotEmpty) {
      _collectInlineItems(child, items);
    }
  }
}

class _TextRun {
  final String text;
  final String fontSize;
  final String fontFamily;
  final String fontWeight;
  final String fontStyle;
  final StyledNode? styledNode;
  final String? linkHref;

  _TextRun({
    required this.text,
    required this.fontSize,
    required this.fontFamily,
    required this.fontWeight,
    required this.fontStyle,
    this.styledNode,
    this.linkHref,
  });
}


// ── Table layout ────────────────────────────────────────────────────

bool _isTableRow(LayoutBox box) {
  if (box.styledNode?.node is! Element) return false;
  return (box.styledNode!.node as Element).tagName == 'tr';
}

bool _isRowGroup(LayoutBox box) {
  if (box.styledNode?.node is! Element) return false;
  final tag = (box.styledNode!.node as Element).tagName;
  return tag == 'tbody' || tag == 'thead' || tag == 'tfoot';
}

bool _isTableCell(LayoutBox box) {
  if (box.styledNode?.node is! Element) return false;
  final tag = (box.styledNode!.node as Element).tagName;
  return tag == 'td' || tag == 'th';
}

/// Get table cell children of a row, filtering out whitespace-only text nodes.
List<LayoutBox> _getTableCells(LayoutBox row) {
  return row.children.where((c) {
    if (_isTableCell(c)) return true;
    // Include non-cell block children that are actual content.
    if (c.layoutType == LayoutType.block && c.styledNode?.node is Element) return true;
    return false;
  }).toList();
}

/// Collect all <tr> rows from a table, walking through tbody/thead/tfoot.
List<LayoutBox> _collectTableRows(LayoutBox table) {
  final rows = <LayoutBox>[];
  for (final child in table.children) {
    if (_isTableRow(child)) {
      rows.add(child);
    } else if (_isRowGroup(child)) {
      for (final gc in child.children) {
        if (_isTableRow(gc)) rows.add(gc);
      }
    }
  }
  return rows;
}

/// Full table layout: determines consistent column widths across all rows
/// and positions cells in a grid pattern.
void _layoutTable(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  final rows = _collectTableRows(box);

  if (rows.isEmpty) {
    // No recognizable table structure — fall back to block layout.
    if (_hasInlineChildren(box)) {
      _layoutInlineChildren(box, containerWidth, measurer);
    } else {
      _layoutBlockChildren(box, containerWidth, measurer);
    }
    return;
  }

  // Read cellspacing from the <table> element.
  double cellSpacing = 2.0; // Default cellspacing.
  if (box.styledNode?.node is Element) {
    final el = box.styledNode!.node as Element;
    final cs = el.attributes['cellspacing'];
    if (cs != null && cs.isNotEmpty) {
      cellSpacing = double.tryParse(cs) ?? 2.0;
    }
    // CSS border-spacing overrides the attribute.
    final bsStr = box.styledNode!.prop('border-spacing', '');
    if (bsStr.isNotEmpty) {
      cellSpacing = _parsePx(bsStr);
    }
  }

  // Determine column count (max cells in any row).
  int colCount = 0;
  for (final row in rows) {
    final cells = _getTableCells(row);
    int cols = 0;
    for (final cell in cells) {
      final colspan = _getColspan(cell);
      cols += colspan;
    }
    colCount = math.max(colCount, cols);
  }
  if (colCount == 0) return;

  // Available width for cells after spacing.
  final totalSpacing = cellSpacing * (colCount + 1);
  final availableForCells = math.max(0.0, containerWidth - totalSpacing);

  // Calculate column widths — collect explicit widths from cells.
  final colWidths = List<double>.filled(colCount, -1.0);
  for (final row in rows) {
    final cells = _getTableCells(row);
    int colIdx = 0;
    for (final cell in cells) {
      if (colIdx >= colCount) break;
      final colspan = _getColspan(cell);
      if (colspan == 1 && colWidths[colIdx] < 0) {
        final widthStr = cell.styledNode?.prop('width', '') ?? '';
        if (widthStr.isNotEmpty && widthStr != 'auto') {
          final w = _parsePx(widthStr, availableForCells);
          if (w > 0) colWidths[colIdx] = w;
        }
      }
      colIdx += colspan;
    }
  }

  // Distribute remaining width among auto columns.
  double totalFixed = 0;
  int autoCount = 0;
  for (int i = 0; i < colCount; i++) {
    if (colWidths[i] >= 0) {
      totalFixed += colWidths[i];
    } else {
      autoCount++;
    }
  }
  final remaining = math.max(0.0, availableForCells - totalFixed);
  final autoWidth = autoCount > 0 ? remaining / autoCount : 0.0;
  for (int i = 0; i < colCount; i++) {
    if (colWidths[i] < 0) colWidths[i] = autoWidth;
  }

  // Layout each row using the computed column widths.
  double cursorY = box.content.y + cellSpacing;

  for (final child in box.children) {
    if (_isTableRow(child)) {
      cursorY = _layoutTableRowWithCols(
          child, box, colWidths, cursorY, cellSpacing, measurer);
    } else if (_isRowGroup(child)) {
      // Row groups are transparent — process their row children.
      _computeBoxDimensions(child, containerWidth);
      child.content.x = box.content.x;
      child.content.y = cursorY;
      child.content.width = containerWidth;
      final groupStartY = cursorY;
      for (final gc in child.children) {
        if (_isTableRow(gc)) {
          cursorY = _layoutTableRowWithCols(
              gc, box, colWidths, cursorY, cellSpacing, measurer);
        }
      }
      child.content.height = cursorY - groupStartY;
    }
  }

  box.content.height = cursorY - box.content.y;
}

/// Get the colspan value from a table cell.
int _getColspan(LayoutBox cell) {
  if (cell.styledNode?.node is! Element) return 1;
  final el = cell.styledNode!.node as Element;
  final cs = el.attributes['colspan'];
  if (cs == null || cs.isEmpty) return 1;
  return int.tryParse(cs) ?? 1;
}

/// Layout a single table row using pre-computed column widths.
/// Returns the Y position after this row.
double _layoutTableRowWithCols(
  LayoutBox row,
  LayoutBox table,
  List<double> colWidths,
  double startY,
  double cellSpacing,
  TextMeasurer measurer,
) {
  _computeBoxDimensions(row, table.content.width);
  row.content.x = table.content.x;
  row.content.y = startY;
  row.content.width = table.content.width;

  final cells = _getTableCells(row);
  double cursorX = table.content.x + cellSpacing;
  double maxCellHeight = 0;

  int colIdx = 0;
  for (int i = 0; i < cells.length; i++) {
    if (colIdx >= colWidths.length) break;
    final cell = cells[i];
    final colspan = _getColspan(cell);

    // Calculate total width for this cell (sum of spanned columns + spacing).
    double cellWidth = 0;
    for (int c = 0; c < colspan && colIdx + c < colWidths.length; c++) {
      cellWidth += colWidths[colIdx + c];
      if (c > 0) cellWidth += cellSpacing;
    }

    _computeBoxDimensions(cell, cellWidth);
    cell.content.width = math.max(0, cellWidth -
        cell.margin.left - cell.margin.right -
        cell.border.left - cell.border.right -
        cell.padding.left - cell.padding.right);
    cell.content.x = cursorX +
        cell.margin.left + cell.border.left + cell.padding.left;
    cell.content.y = startY +
        cell.margin.top + cell.border.top + cell.padding.top;

    // Layout cell content.
    if (cell.layoutType == LayoutType.flex) {
      _layoutFlex(cell, cell.content.width, measurer);
    } else if (cell.layoutType == LayoutType.table) {
      _layoutTable(cell, cell.content.width, measurer);
    } else if (_hasInlineChildren(cell)) {
      _layoutInlineChildren(cell, cell.content.width, measurer);
    } else {
      _layoutBlockChildren(cell, cell.content.width, measurer);
    }

    // Auto-height for the cell.
    final heightProp = cell.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final ch in cell.children) {
        h = math.max(h, ch.marginBox.y + ch.marginBox.height - cell.content.y);
      }
      cell.content.height = h;
    }
    _applyHeightConstraints(cell);

    final cellTotalHeight = cell.content.height +
        cell.margin.top + cell.margin.bottom +
        cell.border.top + cell.border.bottom +
        cell.padding.top + cell.padding.bottom;
    maxCellHeight = math.max(maxCellHeight, cellTotalHeight);

    // Advance horizontal cursor past this cell's columns.
    for (int c = 0; c < colspan && colIdx + c < colWidths.length; c++) {
      cursorX += colWidths[colIdx + c] + cellSpacing;
    }
    colIdx += colspan;
  }

  row.content.height = maxCellHeight;
  return startY + maxCellHeight + cellSpacing;
}

/// Legacy standalone row layout (used when <tr> appears outside <table>).
void _layoutTableRow(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  if (box.children.isEmpty) return;

  final cells = _getTableCells(box);
  if (cells.isEmpty) {
    // No cells — fall back to block layout.
    _layoutBlockChildren(box, containerWidth, measurer);
    return;
  }

  final cellCount = cells.length;
  final cellWidths = List<double>.filled(cellCount, -1.0);
  double totalFixed = 0;
  int autoCount = 0;

  for (int i = 0; i < cellCount; i++) {
    final cell = cells[i];
    final widthStr = cell.styledNode?.prop('width', '') ?? '';
    if (widthStr.isNotEmpty && widthStr != 'auto') {
      final w = _parsePx(widthStr, containerWidth);
      if (w > 0) {
        cellWidths[i] = w;
        totalFixed += w;
        continue;
      }
    }
    autoCount++;
  }

  final remaining = math.max(0.0, containerWidth - totalFixed);
  final autoWidth = autoCount > 0 ? remaining / autoCount : 0.0;
  for (int i = 0; i < cellCount; i++) {
    if (cellWidths[i] < 0) cellWidths[i] = autoWidth;
  }

  double cursorX = box.content.x;

  for (int i = 0; i < cellCount; i++) {
    final cell = cells[i];
    final allocatedWidth = cellWidths[i];

    _computeBoxDimensions(cell, allocatedWidth);
    cell.content.width = math.max(0, allocatedWidth -
        cell.margin.left - cell.margin.right -
        cell.border.left - cell.border.right -
        cell.padding.left - cell.padding.right);
    cell.content.x = cursorX +
        cell.margin.left + cell.border.left + cell.padding.left;
    cell.content.y = box.content.y +
        cell.margin.top + cell.border.top + cell.padding.top;

    if (cell.layoutType == LayoutType.table) {
      _layoutTable(cell, cell.content.width, measurer);
    } else if (_hasInlineChildren(cell)) {
      _layoutInlineChildren(cell, cell.content.width, measurer);
    } else {
      _layoutBlockChildren(cell, cell.content.width, measurer);
    }

    final heightProp = cell.styledNode?.prop('height', '') ?? '';
    if (heightProp.isEmpty || heightProp == 'auto') {
      double h = 0;
      for (final child in cell.children) {
        h = math.max(h, child.marginBox.y + child.marginBox.height - cell.content.y);
      }
      cell.content.height = h;
    }

    cursorX += allocatedWidth;
  }
}

// ── CSS value parsing helpers ───────────────────────────────────────

double _parsePx(String value, [double fallback = 0]) {
  if (value.isEmpty) return fallback;

  // Handle calc() expressions.
  if (value.startsWith('calc(') && value.endsWith(')')) {
    return _evaluateCalc(value.substring(5, value.length - 1), fallback);
  }

  // Handle "Xpx".
  if (value.endsWith('px')) {
    return double.tryParse(value.replaceAll('px', '')) ?? fallback;
  }

  // Handle "Xrem" — relative to root 16px.
  if (value.endsWith('rem')) {
    final n = double.tryParse(value.replaceAll('rem', ''));
    if (n != null) return n * 16;
  }

  // Handle "Xem" — relative to base 16px.
  if (value.endsWith('em')) {
    final n = double.tryParse(value.replaceAll('em', ''));
    if (n != null) return n * 16;
  }

  // Handle viewport units (approximate with 1024x768 default).
  if (value.endsWith('vw')) {
    final n = double.tryParse(value.replaceAll('vw', ''));
    if (n != null) return n / 100 * 1024;
  }
  if (value.endsWith('vh')) {
    final n = double.tryParse(value.replaceAll('vh', ''));
    if (n != null) return n / 100 * 768;
  }
  if (value.endsWith('vmin')) {
    final n = double.tryParse(value.replaceAll('vmin', ''));
    if (n != null) return n / 100 * 768;
  }
  if (value.endsWith('vmax')) {
    final n = double.tryParse(value.replaceAll('vmax', ''));
    if (n != null) return n / 100 * 1024;
  }

  // Handle "Xpt" — 1pt = 1.333px.
  if (value.endsWith('pt')) {
    final n = double.tryParse(value.replaceAll('pt', ''));
    if (n != null) return n * 1.333;
  }

  // Handle "Xcm" — 1cm = 37.795px.
  if (value.endsWith('cm')) {
    final n = double.tryParse(value.replaceAll('cm', ''));
    if (n != null) return n * 37.795;
  }

  // Handle "Xmm" — 1mm = 3.7795px.
  if (value.endsWith('mm')) {
    final n = double.tryParse(value.replaceAll('mm', ''));
    if (n != null) return n * 3.7795;
  }

  // Handle "Xin" — 1in = 96px.
  if (value.endsWith('in')) {
    final n = double.tryParse(value.replaceAll('in', ''));
    if (n != null) return n * 96;
  }

  // Handle "X%" — relative to container.
  if (value.endsWith('%')) {
    final n = double.tryParse(value.replaceAll('%', ''));
    if (n != null) return n / 100 * fallback;
  }

  // Handle bare numbers.
  return double.tryParse(value) ?? fallback;
}

EdgeSizes _parseEdges(StyledNode s, String property) {
  // Start from shorthand, then let longhands override.
  final shorthand = s.prop(property, '');
  double top, right, bottom, left;
  if (shorthand.isNotEmpty) {
    final base = _parseShorthand(shorthand);
    top = base.top;
    right = base.right;
    bottom = base.bottom;
    left = base.left;
  } else {
    top = 0;
    right = 0;
    bottom = 0;
    left = 0;
  }
  // Individual longhands override the shorthand values.
  final topStr = s.prop('$property-top', '');
  final rightStr = s.prop('$property-right', '');
  final bottomStr = s.prop('$property-bottom', '');
  final leftStr = s.prop('$property-left', '');
  if (topStr.isNotEmpty) top = _parsePx(topStr);
  if (rightStr.isNotEmpty) right = _parsePx(rightStr);
  if (bottomStr.isNotEmpty) bottom = _parsePx(bottomStr);
  if (leftStr.isNotEmpty) left = _parsePx(leftStr);
  return EdgeSizes(top, right, bottom, left);
}

EdgeSizes _parseShorthand(String value) {
  final parts = value.trim().split(RegExp(r'\s+')).map((p) => _parsePx(p)).toList();
  switch (parts.length) {
    case 1:
      return EdgeSizes(parts[0], parts[0], parts[0], parts[0]);
    case 2:
      return EdgeSizes(parts[0], parts[1], parts[0], parts[1]);
    case 3:
      return EdgeSizes(parts[0], parts[1], parts[2], parts[1]);
    case 4:
      return EdgeSizes(parts[0], parts[1], parts[2], parts[3]);
    default:
      return EdgeSizes.zero;
  }
}

EdgeSizes _parseBorderWidths(StyledNode s) {
  double top = 0, right = 0, bottom = 0, left = 0;

  final borderAll = s.prop('border', '');
  if (borderAll.isNotEmpty) {
    final w = _parseBorderSide(borderAll);
    top = right = bottom = left = w;
  }

  // Individual border-width values.
  final bwStr = s.prop('border-width', '');
  if (bwStr.isNotEmpty) {
    final parts = bwStr.trim().split(RegExp(r'\s+'));
    switch (parts.length) {
      case 1:
        top = right = bottom = left = _parsePx(parts[0]);
      case 2:
        top = bottom = _parsePx(parts[0]);
        right = left = _parsePx(parts[1]);
      case 3:
        top = _parsePx(parts[0]);
        right = left = _parsePx(parts[1]);
        bottom = _parsePx(parts[2]);
      case 4:
        top = _parsePx(parts[0]);
        right = _parsePx(parts[1]);
        bottom = _parsePx(parts[2]);
        left = _parsePx(parts[3]);
    }
  }

  final bt = s.prop('border-top', '');
  if (bt.isNotEmpty) top = _parseBorderSide(bt);
  final br = s.prop('border-right', '');
  if (br.isNotEmpty) right = _parseBorderSide(br);
  final bb = s.prop('border-bottom', '');
  if (bb.isNotEmpty) bottom = _parseBorderSide(bb);
  final bl = s.prop('border-left', '');
  if (bl.isNotEmpty) left = _parseBorderSide(bl);

  // Individual width overrides.
  final btw = s.prop('border-top-width', '');
  if (btw.isNotEmpty) top = _parsePx(btw);
  final brw = s.prop('border-right-width', '');
  if (brw.isNotEmpty) right = _parsePx(brw);
  final bbw = s.prop('border-bottom-width', '');
  if (bbw.isNotEmpty) bottom = _parsePx(bbw);
  final blw = s.prop('border-left-width', '');
  if (blw.isNotEmpty) left = _parsePx(blw);

  return EdgeSizes(top, right, bottom, left);
}

/// Evaluate a calc() expression like "100% - 20px" or "50vw + 2rem".
/// Supports +, -, *, / with standard operator precedence.
double _evaluateCalc(String expr, double percentBase) {
  final tokens = _tokenizeCalc(expr.trim());
  if (tokens.isEmpty) return 0;
  return _parseCalcAddSub(tokens, 0, percentBase).$1;
}

/// Tokenize a calc expression into numbers-with-units and operators.
List<String> _tokenizeCalc(String expr) {
  final tokens = <String>[];
  int i = 0;
  while (i < expr.length) {
    final c = expr[i];
    if (c == ' ' || c == '\t') { i++; continue; }
    if (c == '(') {
      // Find matching close paren.
      int depth = 1;
      int start = i + 1;
      i++;
      while (i < expr.length && depth > 0) {
        if (expr[i] == '(') depth++;
        if (expr[i] == ')') depth--;
        i++;
      }
      tokens.add('(${expr.substring(start, i - 1)})');
    } else if (c == '+' || c == '-') {
      // Distinguish unary minus from binary minus.
      if (tokens.isNotEmpty && !_isCalcOp(tokens.last)) {
        tokens.add(c);
        i++;
      } else {
        // Unary: part of number.
        final start = i;
        i++;
        while (i < expr.length && (RegExp(r'[0-9a-zA-Z.%]').hasMatch(expr[i]))) i++;
        tokens.add(expr.substring(start, i));
      }
    } else if (c == '*' || c == '/') {
      tokens.add(c);
      i++;
    } else {
      // Number with optional unit.
      final start = i;
      while (i < expr.length && expr[i] != ' ' && expr[i] != '+' && expr[i] != '-' &&
          expr[i] != '*' && expr[i] != '/' && expr[i] != ')' && expr[i] != '(') {
        i++;
      }
      tokens.add(expr.substring(start, i));
    }
  }
  return tokens;
}

bool _isCalcOp(String s) => s == '+' || s == '-' || s == '*' || s == '/';

/// Parse addition/subtraction (lowest precedence).
(double, int) _parseCalcAddSub(List<String> tokens, int pos, double percentBase) {
  var (value, i) = _parseCalcMulDiv(tokens, pos, percentBase);
  while (i < tokens.length) {
    final op = tokens[i];
    if (op != '+' && op != '-') break;
    i++;
    final (right, ni) = _parseCalcMulDiv(tokens, i, percentBase);
    i = ni;
    value = op == '+' ? value + right : value - right;
  }
  return (value, i);
}

/// Parse multiplication/division (higher precedence).
(double, int) _parseCalcMulDiv(List<String> tokens, int pos, double percentBase) {
  var (value, i) = _parseCalcAtom(tokens, pos, percentBase);
  while (i < tokens.length) {
    final op = tokens[i];
    if (op != '*' && op != '/') break;
    i++;
    final (right, ni) = _parseCalcAtom(tokens, i, percentBase);
    i = ni;
    value = op == '*' ? value * right : (right != 0 ? value / right : 0);
  }
  return (value, i);
}

/// Parse an atom: a number with unit, or a parenthesized sub-expression.
(double, int) _parseCalcAtom(List<String> tokens, int pos, double percentBase) {
  if (pos >= tokens.length) return (0, pos);
  final token = tokens[pos];
  if (token.startsWith('(') && token.endsWith(')')) {
    final inner = token.substring(1, token.length - 1);
    return (_evaluateCalc(inner, percentBase), pos + 1);
  }
  return (_parsePxCalcValue(token, percentBase), pos + 1);
}

/// Parse a single value with unit inside calc. Handles %, px, em, rem, vw, vh, etc.
double _parsePxCalcValue(String value, double percentBase) {
  if (value.endsWith('%')) {
    final n = double.tryParse(value.replaceAll('%', ''));
    if (n != null) return n / 100 * percentBase;
    return 0;
  }
  if (value.endsWith('px')) return double.tryParse(value.replaceAll('px', '')) ?? 0;
  if (value.endsWith('rem')) { final n = double.tryParse(value.replaceAll('rem', '')); return n != null ? n * 16 : 0; }
  if (value.endsWith('em')) { final n = double.tryParse(value.replaceAll('em', '')); return n != null ? n * 16 : 0; }
  if (value.endsWith('vw')) { final n = double.tryParse(value.replaceAll('vw', '')); return n != null ? n / 100 * 1024 : 0; }
  if (value.endsWith('vh')) { final n = double.tryParse(value.replaceAll('vh', '')); return n != null ? n / 100 * 768 : 0; }
  if (value.endsWith('pt')) { final n = double.tryParse(value.replaceAll('pt', '')); return n != null ? n * 1.333 : 0; }
  return double.tryParse(value) ?? 0;
}

double _parseBorderSide(String value) {
  final parts = value.trim().split(RegExp(r'\s+'));
  if (parts.isNotEmpty) return _parsePx(parts[0]);
  return 0;
}
