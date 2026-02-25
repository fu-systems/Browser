import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';
import 'package:pane/engine/css.dart';
import 'package:pane/engine/style.dart';
import 'package:pane/engine/layout.dart';

/// Mock text measurer that uses a fixed character width for testing.
class MockTextMeasurer implements TextMeasurer {
  final double charWidth;
  final double lineHeight;

  MockTextMeasurer({this.charWidth = 8.0, this.lineHeight = 20.0});

  @override
  TextMetrics measureText(
    String text, {
    required double fontSize,
    required String fontFamily,
    required String fontWeight,
    required String fontStyle,
    required double maxWidth,
  }) {
    if (text.isEmpty) return TextMetrics(0, 0, []);

    final scaledCharWidth = charWidth * (fontSize / 16.0);
    final scaledLineHeight = lineHeight * (fontSize / 16.0);

    // Simple word-wrapping.
    final lines = <TextLine>[];
    final words = text.split(' ');
    var currentLine = StringBuffer();
    var currentWidth = 0.0;

    for (final word in words) {
      final wordWidth = word.length * scaledCharWidth;
      final spaceWidth = currentLine.isEmpty ? 0.0 : scaledCharWidth;

      if (currentWidth + spaceWidth + wordWidth > maxWidth && currentLine.isNotEmpty) {
        final lineText = currentLine.toString();
        lines.add(TextLine(lineText, currentWidth, scaledLineHeight, scaledLineHeight * 0.8));
        currentLine = StringBuffer(word);
        currentWidth = wordWidth;
      } else {
        if (currentLine.isNotEmpty) {
          currentLine.write(' ');
          currentWidth += scaledCharWidth;
        }
        currentLine.write(word);
        currentWidth += wordWidth;
      }
    }

    if (currentLine.isNotEmpty) {
      lines.add(TextLine(currentLine.toString(), currentWidth, scaledLineHeight, scaledLineHeight * 0.8));
    }

    final totalWidth = lines.isEmpty ? 0.0 : lines.map((l) => l.width).reduce((a, b) => a > b ? a : b);
    final totalHeight = lines.length * scaledLineHeight;

    return TextMetrics(totalWidth, totalHeight, lines);
  }
}

void main() {
  late MockTextMeasurer measurer;

  setUp(() {
    measurer = MockTextMeasurer();
  });

  group('Layout engine', () {
    test('creates a layout tree from styled tree', () {
      final doc = HtmlParser.parse('<body><p>Hello World</p></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      expect(root, isNotNull);
      expect(root.content.width, 800);
    });

    test('block elements stack vertically', () {
      final doc = HtmlParser.parse('<body><p>First</p><p>Second</p></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      // Should have children (the p elements).
      expect(root.children.length, greaterThanOrEqualTo(2));
    });

    test('layout respects explicit width', () {
      final doc = HtmlParser.parse('<body><div style="width:400px">Content</div></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final div = _findLayoutBox(root, 'div');
      expect(div, isNotNull);
      expect(div!.content.width, 400);
    });

    test('layout respects explicit height', () {
      final doc = HtmlParser.parse('<body><div style="height:200px">Content</div></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final div = _findLayoutBox(root, 'div');
      expect(div, isNotNull);
      expect(div!.content.height, 200);
    });

    test('layout applies margin', () {
      final doc = HtmlParser.parse('<body><div style="margin:20px">Content</div></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final div = _findLayoutBox(root, 'div');
      expect(div, isNotNull);
      expect(div!.margin.top, 20);
      expect(div!.margin.right, 20);
      expect(div!.margin.bottom, 20);
      expect(div!.margin.left, 20);
    });

    test('layout applies padding', () {
      final doc = HtmlParser.parse('<body><div style="padding:10px">Content</div></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final div = _findLayoutBox(root, 'div');
      expect(div, isNotNull);
      expect(div!.padding.top, 10);
      expect(div!.padding.left, 10);
    });

    test('link href propagates to layout boxes', () {
      final doc = HtmlParser.parse('<body><a href="https://example.com">Click me</a></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      // Find a box with linkHref set.
      final linkBox = root.allBoxes.where((b) => b.linkHref != null).firstOrNull;
      expect(linkBox, isNotNull);
      expect(linkBox!.linkHref, 'https://example.com');
    });

    test('page height is computed from content', () {
      final doc = HtmlParser.parse('<body><p>Line 1</p><p>Line 2</p><p>Line 3</p></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      expect(root.content.height, greaterThan(0));
    });
  });
}

LayoutBox? _findLayoutBox(LayoutBox root, String tagName) {
  for (final box in root.allBoxes) {
    if (box.styledNode?.node is Element &&
        (box.styledNode!.node as Element).tagName == tagName) {
      return box;
    }
  }
  return null;
}
