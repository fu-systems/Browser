import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';
import 'package:pane/engine/css.dart';
import 'package:pane/engine/style.dart';
import 'package:pane/engine/layout.dart';
import 'package:pane/ui/tab.dart';

/// Mock text measurer for testing.
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

  group('Phase 2 — Image layout', () {
    test('img elements produce layout boxes with image URL', () {
      final doc = HtmlParser.parse('<body><img src="photo.jpg" width="200" height="100"></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final imgBox = root.allBoxes.where((b) => b.imageUrl != null).firstOrNull;
      expect(imgBox, isNotNull);
      expect(imgBox!.imageUrl, 'photo.jpg');
    });

    test('img elements use specified width/height', () {
      final doc = HtmlParser.parse('<body><img src="photo.jpg" width="200" height="100"></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final imgBox = root.allBoxes.where((b) => b.imageUrl != null).first;
      expect(imgBox.imageWidth, 200);
      expect(imgBox.imageHeight, 100);
    });

    test('img elements without dimensions use defaults', () {
      final doc = HtmlParser.parse('<body><img src="photo.jpg"></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final imgBox = root.allBoxes.where((b) => b.imageUrl != null).first;
      expect(imgBox.imageWidth, 300);
      expect(imgBox.imageHeight, 150);
    });
  });

  group('Phase 2 — Form element layout', () {
    test('input element produces form layout box', () {
      final doc = HtmlParser.parse('<body><input type="text" placeholder="Name"></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final formBox = root.allBoxes.where((b) => b.formTag == 'input').firstOrNull;
      expect(formBox, isNotNull);
      expect(formBox!.formType, 'text');
      expect(formBox.formPlaceholder, 'Name');
    });

    test('button element produces form layout box', () {
      final doc = HtmlParser.parse('<body><button>Click Me</button></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final formBox = root.allBoxes.where((b) => b.formTag == 'button').firstOrNull;
      expect(formBox, isNotNull);
      expect(formBox!.formType, 'button');
    });

    test('select element produces form layout box', () {
      final doc = HtmlParser.parse('<body><select><option>A</option></select></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final formBox = root.allBoxes.where((b) => b.formTag == 'select').firstOrNull;
      expect(formBox, isNotNull);
    });

    test('textarea element produces form layout box', () {
      final doc = HtmlParser.parse('<body><textarea placeholder="Enter text"></textarea></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final formBox = root.allBoxes.where((b) => b.formTag == 'textarea').firstOrNull;
      expect(formBox, isNotNull);
      expect(formBox!.formPlaceholder, 'Enter text');
    });

    test('checkbox input produces small form box', () {
      final doc = HtmlParser.parse('<body><input type="checkbox"></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final formBox = root.allBoxes.where((b) => b.formTag == 'input').firstOrNull;
      expect(formBox, isNotNull);
      expect(formBox!.formType, 'checkbox');
    });
  });

  group('Phase 2 — Tab model extensions', () {
    test('tab stores source HTML', () {
      final tab = Tab();
      tab.sourceHtml = '<html><body>Hello</body></html>';
      expect(tab.sourceHtml, contains('Hello'));
    });

    test('tab search state initializes empty', () {
      final tab = Tab();
      expect(tab.searchQuery, '');
      expect(tab.searchRects, isEmpty);
      expect(tab.searchIndex, -1);
    });

    test('tab image cache starts empty', () {
      final tab = Tab();
      expect(tab.imageCache, isEmpty);
    });
  });

  group('Phase 2 — Inline layout with mixed content', () {
    test('mixed text and image in same parent', () {
      final doc = HtmlParser.parse('<body><p>Text <img src="icon.png" width="16" height="16"> more text</p></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      // Should have at least one text box and one image box.
      final hasText = root.allBoxes.any((b) => b.text != null && b.text!.isNotEmpty);
      final hasImage = root.allBoxes.any((b) => b.imageUrl != null);
      expect(hasText, isTrue);
      expect(hasImage, isTrue);
    });

    test('form elements alongside text', () {
      final doc = HtmlParser.parse('<body><p>Name: <input type="text"></p></body>');
      final styled = computeStyles(doc.body!, []);
      final root = layoutTree(styled, 800, measurer);

      final hasText = root.allBoxes.any((b) => b.text != null && b.text!.isNotEmpty);
      final hasForm = root.allBoxes.any((b) => b.formTag != null);
      expect(hasText, isTrue);
      expect(hasForm, isTrue);
    });
  });
}
