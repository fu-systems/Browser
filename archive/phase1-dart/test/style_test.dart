import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';
import 'package:pane/engine/css.dart';
import 'package:pane/engine/style.dart';

void main() {
  group('Style resolution', () {
    test('applies default styles to elements', () {
      final doc = HtmlParser.parse('<body><h1>Title</h1></body>');
      final styled = computeStyles(doc.body!, []);

      // Find the h1 styled node.
      final h1 = _findStyledElement(styled, 'h1');
      expect(h1, isNotNull);
      expect(h1!['font-weight'], 'bold');
      expect(h1['font-size'], '32px');
    });

    test('applies CSS rules to matching elements', () {
      final doc = HtmlParser.parse('<body><p>Hello</p></body>');
      final sheet = CssParser.parse('p { color: red; }');
      final styled = computeStyles(doc.body!, [sheet]);

      final p = _findStyledElement(styled, 'p');
      expect(p, isNotNull);
      expect(p!['color'], 'red');
    });

    test('higher specificity wins', () {
      final doc = HtmlParser.parse('<body><p class="special">Hello</p></body>');
      final sheet = CssParser.parse('''
        p { color: red; }
        .special { color: blue; }
      ''');
      final styled = computeStyles(doc.body!, [sheet]);

      final p = _findStyledElement(styled, 'p');
      expect(p!['color'], 'blue');
    });

    test('inline styles override stylesheet rules', () {
      final doc = HtmlParser.parse('<body><p style="color: green;">Hello</p></body>');
      final sheet = CssParser.parse('p { color: red; }');
      final styled = computeStyles(doc.body!, [sheet]);

      final p = _findStyledElement(styled, 'p');
      expect(p!['color'], 'green');
    });

    test('inheritable properties pass to children', () {
      final doc = HtmlParser.parse('<body><div><span>Text</span></div></body>');
      final sheet = CssParser.parse('div { color: purple; }');
      final styled = computeStyles(doc.body!, [sheet]);

      final span = _findStyledElement(styled, 'span');
      expect(span, isNotNull);
      expect(span!['color'], 'purple');
    });

    test('display:none results in Display.none', () {
      final doc = HtmlParser.parse('<body><div style="display:none">Hidden</div></body>');
      final styled = computeStyles(doc.body!, []);

      final div = _findStyledElement(styled, 'div');
      expect(div, isNotNull);
      expect(div!.display, Display.none);
    });

    test('block elements default to Display.block', () {
      final doc = HtmlParser.parse('<body><div>Block</div></body>');
      final styled = computeStyles(doc.body!, []);

      final div = _findStyledElement(styled, 'div');
      expect(div!.display, Display.block);
    });

    test('inline elements default to Display.inline', () {
      final doc = HtmlParser.parse('<body><span>Inline</span></body>');
      final styled = computeStyles(doc.body!, []);

      final span = _findStyledElement(styled, 'span');
      expect(span!.display, Display.inline);
    });

    test('a tags get default link styles', () {
      final doc = HtmlParser.parse('<body><a href="#">Link</a></body>');
      final styled = computeStyles(doc.body!, []);

      final a = _findStyledElement(styled, 'a');
      expect(a!['color'], '#0000EE');
      expect(a['text-decoration'], 'underline');
    });

    test('descendant selector matches correctly', () {
      final doc = HtmlParser.parse('<body><div><p>Nested</p></div><p>Top</p></body>');
      final sheet = CssParser.parse('div p { color: red; }');
      final styled = computeStyles(doc.body!, [sheet]);

      // The p inside div should have color red.
      final div = _findStyledElement(styled, 'div');
      final nestedP = _findStyledElement(div!, 'p');
      expect(nestedP!['color'], 'red');
    });
  });
}

/// Find a StyledNode for an element with the given tag name.
StyledNode? _findStyledElement(StyledNode root, String tagName) {
  if (root.node is Element && (root.node as Element).tagName == tagName) {
    return root;
  }
  for (final child in root.children) {
    final found = _findStyledElement(child, tagName);
    if (found != null) return found;
  }
  return null;
}
