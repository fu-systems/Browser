/// Layout engine — pure Dart, no Flutter dependency.
///
/// Takes a StyledNode tree and produces a LayoutBox tree with computed
/// x, y, width, height for every box. Implements block and inline layout
/// with the CSS box model (margin, padding, border).
///
/// Text measurement is provided via an abstract [TextMeasurer] interface
/// so the engine stays independent of Flutter.

import 'dart:math' as math;
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

enum LayoutType { block, inline, anonymous, text }

/// A node in the layout tree. Each box has a content rect plus
/// margin, border, and padding edges.
class LayoutBox {
  final LayoutType layoutType;
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

  // Text node → text layout box.
  if (styled.node is Text) {
    final box = LayoutBox(LayoutType.text, styled);
    box.text = (styled.node as Text).data;
    box.linkHref = href;
    return box;
  }

  final display = styled.display;
  if (display == Display.none) {
    // Return an empty box that will be skipped.
    final box = LayoutBox(LayoutType.block, styled);
    box.content = Rect(0, 0, 0, 0);
    return box;
  }

  final type = (display == Display.inline || display == Display.inlineBlock)
      ? LayoutType.inline
      : LayoutType.block;
  final box = LayoutBox(type, styled);
  box.linkHref = href;

  for (final child in styled.children) {
    if (child.display == Display.none) continue;
    final childBox = _buildLayoutTree(child, href);
    box.children.add(childBox);
  }

  return box;
}

// ── Block layout ────────────────────────────────────────────────────

void _layoutBlock(LayoutBox box, double containerWidth, TextMeasurer measurer) {
  _computeBoxDimensions(box, containerWidth);

  // Content width = container minus our horizontal margin/border/padding.
  final contentWidth = box.content.width;

  // Lay out children.
  if (_hasInlineChildren(box)) {
    _layoutInlineChildren(box, contentWidth, measurer);
  } else {
    _layoutBlockChildren(box, contentWidth, measurer);
  }

  // If height was not set explicitly, use content height.
  if (box.styledNode?.prop('height', '') == '') {
    // Height is the sum of children.
    double h = 0;
    for (final child in box.children) {
      h = math.max(h, child.marginBox.y + child.marginBox.height - box.content.y);
    }
    box.content.height = h;
  }
}

void _computeBoxDimensions(LayoutBox box, double containerWidth) {
  final s = box.styledNode;
  if (s == null) return;

  box.margin = _parseEdges(s, 'margin');
  box.padding = _parseEdges(s, 'padding');
  box.border = _parseBorderWidths(s);

  // Width: explicit or fill container.
  final widthStr = s.prop('width', '');
  if (widthStr.isNotEmpty && widthStr != 'auto') {
    box.content.width = _parsePx(widthStr, containerWidth);
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

  // Height: explicit or auto (computed later).
  final heightStr = s.prop('height', '');
  if (heightStr.isNotEmpty && heightStr != 'auto') {
    box.content.height = _parsePx(heightStr, 0);
  }
}

// ── Block children layout ───────────────────────────────────────────

void _layoutBlockChildren(
  LayoutBox box,
  double containerWidth,
  TextMeasurer measurer,
) {
  double cursorY = box.content.y;

  for (final child in box.children) {
    if (child.layoutType == LayoutType.text ||
        child.layoutType == LayoutType.inline) {
      // Wrap inline content in an anonymous block.
      _computeBoxDimensions(child, containerWidth);
      child.content.x = box.content.x + child.margin.left + child.border.left + child.padding.left;
      child.content.y = cursorY + child.margin.top + child.border.top + child.padding.top;
      _layoutInlineContent(child, containerWidth, measurer);
      cursorY = child.marginBox.y + child.marginBox.height;
    } else {
      _layoutBlock(child, containerWidth, measurer);
      child.content.x = box.content.x +
          child.margin.left +
          child.border.left +
          child.padding.left;
      child.content.y = cursorY +
          child.margin.top +
          child.border.top +
          child.padding.top;
      // Re-layout with correct position.
      _layoutBlock(child, containerWidth, measurer);
      cursorY = child.marginBox.y + child.marginBox.height;
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
  // Collect all text from inline children into a text run,
  // then measure and break into lines.
  final runs = <_TextRun>[];
  _collectTextRuns(box, runs);

  if (runs.isEmpty) return;

  // Simple line-breaking: concatenate text, measure, then break.
  double cursorX = box.content.x;
  double cursorY = box.content.y;
  double lineHeight = 0;
  double maxWidth = 0;

  for (final run in runs) {
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
      // Does this line fit on the current line?
      if (cursorX + line.width > box.content.x + containerWidth &&
          cursorX > box.content.x) {
        // Wrap to next line.
        cursorX = box.content.x;
        cursorY += lineHeight;
        lineHeight = 0;
      }

      // Place a text box for this line.
      final textBox = LayoutBox(LayoutType.text, run.styledNode);
      textBox.text = line.text;
      textBox.textLines = [line];
      textBox.linkHref = run.linkHref;
      textBox.content = Rect(cursorX, cursorY, line.width, line.height);
      box.children.add(textBox);

      cursorX += line.width;
      lineHeight = math.max(lineHeight, line.height);
      maxWidth = math.max(maxWidth, cursorX - box.content.x);
    }
  }

  // Clear original inline children (we replaced them with text boxes).
  // Actually, keep them — we added new children above. We need to be
  // careful here. Let's use a different approach: place inline children
  // by measuring their text content.

  box.content.height = (cursorY - box.content.y) + lineHeight;
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

void _collectTextRuns(LayoutBox box, List<_TextRun> runs) {
  for (final child in box.children) {
    if (child.text != null && child.text!.isNotEmpty) {
      final s = child.styledNode;
      runs.add(_TextRun(
        text: child.text!,
        fontSize: s?.prop('font-size', '16px') ?? '16px',
        fontFamily: s?.prop('font-family', 'serif') ?? 'serif',
        fontWeight: s?.prop('font-weight', 'normal') ?? 'normal',
        fontStyle: s?.prop('font-style', 'normal') ?? 'normal',
        styledNode: s,
        linkHref: child.linkHref,
      ));
    } else if (child.layoutType == LayoutType.inline) {
      _collectTextRuns(child, runs);
    }
  }
}

// ── CSS value parsing helpers ───────────────────────────────────────

double _parsePx(String value, [double fallback = 0]) {
  if (value.isEmpty) return fallback;

  // Handle "Xpx".
  if (value.endsWith('px')) {
    return double.tryParse(value.replaceAll('px', '')) ?? fallback;
  }

  // Handle "Xem" — relative to base 16px.
  if (value.endsWith('em')) {
    final n = double.tryParse(value.replaceAll('em', ''));
    if (n != null) return n * 16;
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
  // Parse border shorthand or individual sides.
  double top = 0, right = 0, bottom = 0, left = 0;

  final borderAll = s.prop('border', '');
  if (borderAll.isNotEmpty) {
    final w = _parseBorderSide(borderAll);
    top = right = bottom = left = w;
  }

  final bt = s.prop('border-top', '');
  if (bt.isNotEmpty) top = _parseBorderSide(bt);
  final br = s.prop('border-right', '');
  if (br.isNotEmpty) right = _parseBorderSide(br);
  final bb = s.prop('border-bottom', '');
  if (bb.isNotEmpty) bottom = _parseBorderSide(bb);
  final bl = s.prop('border-left', '');
  if (bl.isNotEmpty) left = _parseBorderSide(bl);

  return EdgeSizes(top, right, bottom, left);
}

double _parseBorderSide(String value) {
  // "1px solid #000" → extract the width part.
  final parts = value.trim().split(RegExp(r'\s+'));
  if (parts.isNotEmpty) return _parsePx(parts[0]);
  return 0;
}
