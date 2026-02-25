import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/css.dart';

void main() {
  group('CssParser', () {
    test('parses a simple rule', () {
      final sheet = CssParser.parse('p { color: red; }');
      expect(sheet.rules.length, 1);
      expect(sheet.rules[0].selector.tag, 'p');
      expect(sheet.rules[0].declarations['color'], 'red');
    });

    test('parses multiple declarations', () {
      final sheet = CssParser.parse('h1 { color: blue; font-size: 24px; font-weight: bold; }');
      expect(sheet.rules.length, 1);
      final decls = sheet.rules[0].declarations;
      expect(decls['color'], 'blue');
      expect(decls['font-size'], '24px');
      expect(decls['font-weight'], 'bold');
    });

    test('parses multiple rules', () {
      final sheet = CssParser.parse('''
        p { color: red; }
        h1 { font-size: 32px; }
        .highlight { background-color: yellow; }
      ''');
      expect(sheet.rules.length, 3);
    });

    test('parses class selector', () {
      final sheet = CssParser.parse('.foo { color: green; }');
      expect(sheet.rules[0].selector.classes, ['foo']);
      expect(sheet.rules[0].selector.tag, isNull);
    });

    test('parses id selector', () {
      final sheet = CssParser.parse('#main { width: 100px; }');
      expect(sheet.rules[0].selector.id, 'main');
    });

    test('parses compound selector (tag.class#id)', () {
      final sheet = CssParser.parse('div.container#main { padding: 10px; }');
      final sel = sheet.rules[0].selector;
      expect(sel.tag, 'div');
      expect(sel.classes, ['container']);
      expect(sel.id, 'main');
    });

    test('parses comma-separated selectors into separate rules', () {
      final sheet = CssParser.parse('h1, h2, h3 { color: blue; }');
      expect(sheet.rules.length, 3);
      expect(sheet.rules[0].selector.tag, 'h1');
      expect(sheet.rules[1].selector.tag, 'h2');
      expect(sheet.rules[2].selector.tag, 'h3');
    });

    test('parses descendant combinator', () {
      final sheet = CssParser.parse('div p { margin: 0; }');
      final sel = sheet.rules[0].selector;
      expect(sel.tag, 'p');
      expect(sel.ancestor, isNotNull);
      expect(sel.ancestor!.tag, 'div');
    });

    test('skips comments', () {
      final sheet = CssParser.parse('''
        /* This is a comment */
        p { color: red; }
        /* Another comment */
      ''');
      expect(sheet.rules.length, 1);
    });

    test('skips @-rules', () {
      final sheet = CssParser.parse('''
        @media screen { p { color: red; } }
        h1 { font-size: 32px; }
      ''');
      // @media block is skipped, only h1 rule remains.
      expect(sheet.rules.length, 1);
      expect(sheet.rules[0].selector.tag, 'h1');
    });

    test('handles rgb() color values', () {
      final sheet = CssParser.parse('p { color: rgb(255, 0, 0); }');
      expect(sheet.rules[0].declarations['color'], 'rgb(255, 0, 0)');
    });

    test('strips !important', () {
      final sheet = CssParser.parse('p { color: red !important; }');
      expect(sheet.rules[0].declarations['color'], 'red');
    });
  });

  group('CssParser.parseInlineStyle', () {
    test('parses inline style string', () {
      final decls = CssParser.parseInlineStyle('color: red; font-size: 14px;');
      expect(decls['color'], 'red');
      expect(decls['font-size'], '14px');
    });
  });

  group('Selector specificity', () {
    test('tag selector has specificity (0,0,1)', () {
      final sel = Selector(tag: 'p');
      expect(sel.specificity.a, 0);
      expect(sel.specificity.b, 0);
      expect(sel.specificity.c, 1);
    });

    test('class selector has specificity (0,1,0)', () {
      final sel = Selector(classes: ['foo']);
      expect(sel.specificity.a, 0);
      expect(sel.specificity.b, 1);
      expect(sel.specificity.c, 0);
    });

    test('id selector has specificity (1,0,0)', () {
      final sel = Selector(id: 'main');
      expect(sel.specificity.a, 1);
      expect(sel.specificity.b, 0);
      expect(sel.specificity.c, 0);
    });

    test('#id.class tag has specificity (1,1,1)', () {
      final sel = Selector(tag: 'div', id: 'main', classes: ['foo']);
      expect(sel.specificity.a, 1);
      expect(sel.specificity.b, 1);
      expect(sel.specificity.c, 1);
    });

    test('specificity comparison works', () {
      final idSel = Selector(id: 'x');
      final classSel = Selector(classes: ['x']);
      final tagSel = Selector(tag: 'p');

      expect(idSel.specificity > classSel.specificity, isTrue);
      expect(classSel.specificity > tagSel.specificity, isTrue);
    });
  });
}
