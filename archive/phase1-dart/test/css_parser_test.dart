import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/css.dart';

void main() {
  group('CssParser', () {
    test('parses a simple rule', () {
      final sheet = CssParser.parse('p { color: red; }');
      expect(sheet.rules.length, 1);
      expect(sheet.rules[0].selector.compounds[0].tag, 'p');
      expect(sheet.rules[0].declarations['color']?.value, 'red');
    });

    test('parses multiple declarations', () {
      final sheet = CssParser.parse('h1 { color: blue; font-size: 24px; font-weight: bold; }');
      expect(sheet.rules.length, 1);
      final decls = sheet.rules[0].declarations;
      expect(decls['color']?.value, 'blue');
      expect(decls['font-size']?.value, '24px');
      expect(decls['font-weight']?.value, 'bold');
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
      final compound = sheet.rules[0].selector.compounds[0];
      expect(compound.classes, ['foo']);
      expect(compound.tag, isNull);
    });

    test('parses id selector', () {
      final sheet = CssParser.parse('#main { width: 100px; }');
      expect(sheet.rules[0].selector.compounds[0].id, 'main');
    });

    test('parses compound selector (tag.class#id)', () {
      final sheet = CssParser.parse('div.container#main { padding: 10px; }');
      final compound = sheet.rules[0].selector.compounds[0];
      expect(compound.tag, 'div');
      expect(compound.classes, ['container']);
      expect(compound.id, 'main');
    });

    test('parses comma-separated selectors into separate rules', () {
      final sheet = CssParser.parse('h1, h2, h3 { color: blue; }');
      expect(sheet.rules.length, 3);
      expect(sheet.rules[0].selector.compounds[0].tag, 'h1');
      expect(sheet.rules[1].selector.compounds[0].tag, 'h2');
      expect(sheet.rules[2].selector.compounds[0].tag, 'h3');
    });

    test('parses descendant combinator', () {
      final sheet = CssParser.parse('div p { margin: 0; }');
      final sel = sheet.rules[0].selector;
      // compounds[0] = subject (p), compounds[1] = ancestor (div)
      expect(sel.compounds[0].tag, 'p');
      expect(sel.compounds.length, 2);
      expect(sel.compounds[1].tag, 'div');
      expect(sel.combinators[0], Combinator.descendant);
    });

    test('parses child combinator', () {
      final sheet = CssParser.parse('div > p { margin: 0; }');
      final sel = sheet.rules[0].selector;
      expect(sel.compounds[0].tag, 'p');
      expect(sel.compounds[1].tag, 'div');
      expect(sel.combinators[0], Combinator.child);
    });

    test('parses attribute selector', () {
      final sheet = CssParser.parse('[type="text"] { color: blue; }');
      final compound = sheet.rules[0].selector.compounds[0];
      expect(compound.attributes.length, 1);
      expect(compound.attributes[0].name, 'type');
      expect(compound.attributes[0].op, '=');
      expect(compound.attributes[0].value, 'text');
    });

    test('parses pseudo-class selector', () {
      final sheet = CssParser.parse('a:hover { color: red; }');
      final compound = sheet.rules[0].selector.compounds[0];
      expect(compound.tag, 'a');
      expect(compound.pseudoClasses.length, 1);
      expect(compound.pseudoClasses[0].name, 'hover');
    });

    test('skips comments', () {
      final sheet = CssParser.parse('''
        /* This is a comment */
        p { color: red; }
        /* Another comment */
      ''');
      expect(sheet.rules.length, 1);
    });

    test('parses @media rules', () {
      final sheet = CssParser.parse('''
        @media screen { p { color: red; } }
        h1 { font-size: 32px; }
      ''');
      // @media screen matches (we're always screen), so both rules are included.
      expect(sheet.rules.length, greaterThanOrEqualTo(1));
    });

    test('handles rgb() color values', () {
      final sheet = CssParser.parse('p { color: rgb(255, 0, 0); }');
      expect(sheet.rules[0].declarations['color']?.value, 'rgb(255, 0, 0)');
    });

    test('parses !important flag', () {
      final sheet = CssParser.parse('p { color: red !important; }');
      final decl = sheet.rules[0].declarations['color']!;
      expect(decl.value, 'red');
      expect(decl.important, isTrue);
    });

    test('parses @import directives', () {
      final sheet = CssParser.parse('@import "styles.css"; p { color: red; }');
      expect(sheet.imports.length, 1);
      expect(sheet.imports[0], 'styles.css');
      expect(sheet.rules.length, 1);
    });
  });

  group('CssParser.parseInlineStyle', () {
    test('parses inline style string', () {
      final decls = CssParser.parseInlineStyle('color: red; font-size: 14px;');
      expect(decls['color']?.value, 'red');
      expect(decls['font-size']?.value, '14px');
    });
  });

  group('Selector specificity', () {
    test('tag selector has specificity (0,0,1)', () {
      final sel = _makeSelector(tag: 'p');
      expect(sel.specificity.a, 0);
      expect(sel.specificity.b, 0);
      expect(sel.specificity.c, 1);
    });

    test('class selector has specificity (0,1,0)', () {
      final sel = _makeSelector(classes: ['foo']);
      expect(sel.specificity.a, 0);
      expect(sel.specificity.b, 1);
      expect(sel.specificity.c, 0);
    });

    test('id selector has specificity (1,0,0)', () {
      final sel = _makeSelector(id: 'main');
      expect(sel.specificity.a, 1);
      expect(sel.specificity.b, 0);
      expect(sel.specificity.c, 0);
    });

    test('#id.class tag has specificity (1,1,1)', () {
      final sel = _makeSelector(tag: 'div', id: 'main', classes: ['foo']);
      expect(sel.specificity.a, 1);
      expect(sel.specificity.b, 1);
      expect(sel.specificity.c, 1);
    });

    test('specificity comparison works', () {
      final idSel = _makeSelector(id: 'x');
      final classSel = _makeSelector(classes: ['x']);
      final tagSel = _makeSelector(tag: 'p');

      expect(idSel.specificity > classSel.specificity, isTrue);
      expect(classSel.specificity > tagSel.specificity, isTrue);
    });
  });
}

/// Helper to create a Selector from simple tag/id/classes.
Selector _makeSelector({String? tag, String? id, List<String>? classes}) {
  return Selector(
    [CompoundSelector(tag: tag, id: id, classes: classes ?? const [])],
    [],
  );
}
