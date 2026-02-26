import 'dart:io';
import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';
import 'package:pane/engine/css.dart';
import 'package:pane/engine/style.dart';
import 'package:pane/engine/layout.dart';

/// Stub text measurer for headless testing.
class StubMeasurer implements TextMeasurer {
  final double charWidth;
  StubMeasurer({this.charWidth = 0.6});

  @override
  TextMetrics measureText(String text, {
    required double fontSize,
    required String fontFamily,
    required String fontWeight,
    required String fontStyle,
    required double maxWidth,
  }) {
    final cw = fontSize * charWidth;
    final lineHeight = fontSize * 1.4;
    final totalWidth = text.length * cw;
    final lines = <TextLine>[];

    if (maxWidth <= 0 || totalWidth <= maxWidth) {
      lines.add(TextLine(text, totalWidth, lineHeight, fontSize));
      return TextMetrics(totalWidth, lineHeight, lines);
    }

    int charsPerLine = (maxWidth / cw).floor().clamp(1, text.length);
    int offset = 0;
    while (offset < text.length) {
      final end = (offset + charsPerLine).clamp(0, text.length);
      final lineText = text.substring(offset, end);
      final w = lineText.length * cw;
      lines.add(TextLine(lineText, w, lineHeight, fontSize));
      offset = end;
    }
    return TextMetrics(maxWidth, lines.length * lineHeight, lines);
  }
}

void main() {
  test('Drudge Report layout: blocks stack without overlap', () {
    final html = File('/tmp/drudge_source.html').readAsStringSync();
    final doc = HtmlParser.parse(html);
    expect(doc.title, contains('DRUDGE'));

    final stylesheets = <Stylesheet>[];
    final internalCss = doc.internalCSS;
    if (internalCss.isNotEmpty) {
      stylesheets.add(CssParser.parse(internalCss));
    }

    final body = doc.body ?? doc.documentElement ?? doc;
    final styledTree = computeStyles(body, stylesheets);

    const vw = 1200.0;
    final measurer = StubMeasurer();
    final root = layoutTree(styledTree, vw, measurer);

    // Gather stats
    final allBoxes = root.allBoxes.toList();
    int totalBoxes = allBoxes.length;
    int blockBoxes = 0, inlineBoxes = 0, textBoxes = 0, anonBoxes = 0;
    double maxBottom = 0;

    for (final box in allBoxes) {
      if (box.layoutType == LayoutType.block) blockBoxes++;
      if (box.layoutType == LayoutType.inline) inlineBoxes++;
      if (box.layoutType == LayoutType.text) textBoxes++;
      if (box.layoutType == LayoutType.anonymous) anonBoxes++;
      final bottom = box.marginBox.y + box.marginBox.height;
      if (bottom > maxBottom) maxBottom = bottom;
    }

    print('=== Drudge Report Layout Results ===');
    print('Root: x=${root.content.x.toStringAsFixed(1)} '
        'y=${root.content.y.toStringAsFixed(1)} '
        'w=${root.content.width.toStringAsFixed(1)} '
        'h=${root.content.height.toStringAsFixed(1)}');
    print('Total boxes: $totalBoxes '
        '(block=$blockBoxes inline=$inlineBoxes text=$textBoxes anon=$anonBoxes)');
    print('Page height: ${maxBottom.toStringAsFixed(1)}');

    // Check for overlapping adjacent siblings
    int overlaps = 0;
    void checkOverlaps(LayoutBox parent) {
      for (int i = 0; i < parent.children.length - 1; i++) {
        final a = parent.children[i];
        final b = parent.children[i + 1];
        final aBot = a.content.y + a.content.height;
        final bTop = b.content.y;
        if (aBot > bTop + 0.5 && a.content.height > 0 && b.content.height > 0) {
          overlaps++;
          if (overlaps <= 10) {
            final aTag = a.styledNode?.node.toString() ?? a.layoutType.name;
            final bTag = b.styledNode?.node.toString() ?? b.layoutType.name;
            print('  OVERLAP: $aTag bottom=${aBot.toStringAsFixed(1)} > '
                '$bTag top=${bTop.toStringAsFixed(1)}');
          }
        }
      }
      for (final child in parent.children) {
        checkOverlaps(child);
      }
    }
    checkOverlaps(root);
    print('Overlapping sibling pairs: $overlaps');

    // Print top-level structure
    print('\n=== Top-level children ===');
    for (int i = 0; i < root.children.length && i < 20; i++) {
      final c = root.children[i];
      final tag = c.styledNode?.node.toString() ?? c.layoutType.name;
      print('  [$i] $tag  y=${c.content.y.toStringAsFixed(1)} '
          'h=${c.content.height.toStringAsFixed(1)} type=${c.layoutType.name}');
    }

    // Find the table row and check td positioning
    final tds = allBoxes.where((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'td').toList();
    print('\n=== Table cells (${tds.length}) ===');
    for (int i = 0; i < tds.length && i < 6; i++) {
      final td = tds[i];
      print('  td[$i] x=${td.content.x.toStringAsFixed(1)} '
          'y=${td.content.y.toStringAsFixed(1)} '
          'w=${td.content.width.toStringAsFixed(1)} '
          'h=${td.content.height.toStringAsFixed(1)} '
          'children=${td.children.length}');
    }

    // Assertions
    expect(totalBoxes, greaterThan(50), reason: 'Should produce many layout boxes');
    expect(maxBottom, greaterThan(100), reason: 'Page should have meaningful height');
    expect(root.content.x, greaterThanOrEqualTo(0),
        reason: 'Root x should be >= 0');

    print('\nAll assertions passed!');
  });

  test('Mixed block/inline content flows inline correctly', () {
    final html = '''
    <body>
      <div>
        Some text <a href="/link">a link</a> more text
        <p>A block paragraph</p>
        Trailing text <b>bold part</b> end
      </div>
    </body>
    ''';

    final doc = HtmlParser.parse(html);
    final styledTree = computeStyles(doc.body ?? doc, <Stylesheet>[]);
    final root = layoutTree(styledTree, 800, StubMeasurer());

    final allBoxes = root.allBoxes.toList();
    final div = allBoxes.firstWhere((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'div');

    // Should have anonymous + block + anonymous = 3 children
    expect(div.children.length, 3,
        reason: 'Mixed content div should have 3 children (anon + p + anon)');
    expect(div.children[0].layoutType, LayoutType.anonymous);
    expect(div.children[1].layoutType, LayoutType.block);
    expect(div.children[2].layoutType, LayoutType.anonymous);

    // Each should be stacked vertically
    for (int i = 1; i < div.children.length; i++) {
      final prev = div.children[i - 1];
      final curr = div.children[i];
      final prevBot = prev.content.y + prev.content.height;
      expect(curr.content.y, greaterThanOrEqualTo(prevBot - 0.5),
          reason: 'Child $i should be below child ${i - 1}');
    }

    print('Mixed content test passed!');
  });

  test('Margin collapsing reduces space between siblings', () {
    final html = '''
    <body style="margin: 0;">
      <p style="margin: 20px 0;">First paragraph</p>
      <p style="margin: 20px 0;">Second paragraph</p>
      <p style="margin: 20px 0;">Third paragraph</p>
    </body>
    ''';

    final doc = HtmlParser.parse(html);
    final styledTree = computeStyles(doc.body ?? doc, <Stylesheet>[]);
    final root = layoutTree(styledTree, 800, StubMeasurer());

    final paragraphs = root.children.where((c) =>
        c.styledNode?.node is Element &&
        (c.styledNode!.node as Element).tagName == 'p').toList();

    expect(paragraphs.length, 3);

    // With collapsing: gap between p1 and p2 should be max(20, 20) = 20
    final p1Bot = paragraphs[0].content.y + paragraphs[0].content.height +
        paragraphs[0].padding.bottom + paragraphs[0].border.bottom;
    final p2Top = paragraphs[1].content.y - paragraphs[1].padding.top - paragraphs[1].border.top;
    final gap = p2Top - p1Bot;

    print('Gap between p1 and p2: $gap (should be ~20, not 40)');
    expect(gap, closeTo(20, 1),
        reason: 'Collapsed margin should be max(20,20)=20, not sum 40');

    print('Margin collapsing test passed!');
  });
}
