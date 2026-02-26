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

enum LayoutType { block, inline, anonymous, text, flex }

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
  if (box.layoutType == LayoutType.flex) {
    _layoutFlex(box, contentWidth, measurer);
  } else if (_isTableRow(box)) {
    _layoutTableRow(box, contentWidth, measurer);
  } else if (_hasInlineChildren(box)) {
    _layoutInlineChildren(box, contentWidth, measurer);
  } else {
    _layoutBlockChildren(box, contentWidth, measurer);
  }

  // If height was not set explicitly, use content height.
  final heightProp = box.styledNode?.prop('height', '') ?? '';
  if (heightProp.isEmpty || heightProp == 'auto') {
    double h = 0;
    for (final child in box.children) {
      // Skip absolute/fixed children from contributing to height.
      if (child.position == 'absolute' || child.position == 'fixed') continue;
      h = math.max(h, child.marginBox.y + child.marginBox.height - box.content.y);
    }
    box.content.height = h;
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

// ── Block children layout ───────────────────────────────────────────

void _layoutBlockChildren(
  LayoutBox box,
  double containerWidth,
  TextMeasurer measurer,
) {
  _ensureBlockChildren(box);

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

      // Collapse adjacent vertical margins.
      final collapsed = math.max(prevMarginBottom, child.margin.top);

      if (child.layoutType == LayoutType.text ||
          child.layoutType == LayoutType.inline) {
        child.content.x = box.content.x + child.margin.left + child.border.left + child.padding.left;
        child.content.y = cursorY + collapsed + child.border.top + child.padding.top;
        _layoutInlineContent(child, child.content.width, measurer);
      } else {
        child.content.x = box.content.x +
            child.margin.left +
            child.border.left +
            child.padding.left;
        child.content.y = cursorY +
            collapsed +
            child.border.top +
            child.padding.top;
        _layoutBlock(child, containerWidth, measurer);
      }

      cursorY = child.content.y + child.content.height +
          child.padding.bottom + child.border.bottom;
      prevMarginBottom = child.margin.bottom;
    } catch (_) {
      // Skip this child on error; continue laying out remaining content.
    }
  }
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
      c.layoutType == LayoutType.flex);
  final hasInline = box.children.any((c) =>
      c.layoutType == LayoutType.inline || c.layoutType == LayoutType.text);

  if (!hasBlock || !hasInline) return;

  final newChildren = <LayoutBox>[];
  List<LayoutBox>? inlineRun;

  for (final child in box.children) {
    if (child.layoutType == LayoutType.block ||
        child.layoutType == LayoutType.anonymous ||
        child.layoutType == LayoutType.flex) {
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

/// After flex positioning, update all child positions to be relative
/// to the new parent position.
void _relayoutChildPositions(LayoutBox box) {
  if (box.children.isEmpty) return;
  // Children were laid out with content.x/y = 0, offset them.
  for (final child in box.children) {
    if (child.content.x == 0 && child.content.y == 0) {
      // Already positioned during layout.
    }
  }
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


// ── Table row layout ────────────────────────────────────────────────

bool _isTableRow(LayoutBox box) {
  if (box.styledNode?.node is! Element) return false;
  return (box.styledNode!.node as Element).tagName == 'tr';
}

void _layoutTableRow(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  if (box.children.isEmpty) return;

  final cellCount = box.children.length;
  final cellWidths = List<double>.filled(cellCount, -1.0);
  double totalFixed = 0;
  int autoCount = 0;

  for (int i = 0; i < cellCount; i++) {
    final cell = box.children[i];
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
    final cell = box.children[i];
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

    if (_hasInlineChildren(cell)) {
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
  // Check shorthand first.
  final shorthand = s.prop(property, '');
  if (shorthand.isNotEmpty) {
    return _parseShorthand(shorthand);
  }
  return EdgeSizes(
    _parsePx(s.prop('$property-top', '0')),
    _parsePx(s.prop('$property-right', '0')),
    _parsePx(s.prop('$property-bottom', '0')),
    _parsePx(s.prop('$property-left', '0')),
  );
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

double _parseBorderSide(String value) {
  final parts = value.trim().split(RegExp(r'\s+'));
  if (parts.isNotEmpty) return _parsePx(parts[0]);
  return 0;
}
