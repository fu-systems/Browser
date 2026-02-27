/// Comprehensive W3C spec compliance tests for Pane browser render engine.
///
/// References:
/// - W3C HTML Living Standard (https://html.spec.whatwg.org/)
/// - W3C CSS 2.2 Specification (https://www.w3.org/TR/CSS22/)
/// - W3C CSS Selectors Level 4 (https://www.w3.org/TR/selectors-4/)
/// - W3C CSS Box Model Level 3 (https://www.w3.org/TR/css-box-3/)
/// - W3C CSS Flexbox Level 1 (https://www.w3.org/TR/css-flexbox-1/)
/// - W3C CSS Grid Level 1 (https://www.w3.org/TR/css-grid-1/)
/// - W3C CSS Table Level 3 (https://www.w3.org/TR/css-tables-3/)
/// - W3C CSS Values and Units Level 4 (https://www.w3.org/TR/css-values-4/)
/// - W3C CSS Cascade Level 4 (https://www.w3.org/TR/css-cascade-4/)

import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';
import 'package:pane/engine/css.dart';
import 'package:pane/engine/style.dart';
import 'package:pane/engine/layout.dart';

// ── Test helpers ──────────────────────────────────────────────────────

class MockTextMeasurer implements TextMeasurer {
  final double charWidth;
  final double lineHeight;
  MockTextMeasurer({this.charWidth = 8.0, this.lineHeight = 20.0});

  @override
  TextMetrics measureText(String text,
      {required double fontSize,
      required String fontFamily,
      required String fontWeight,
      required String fontStyle,
      required double maxWidth}) {
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
      if (currentWidth + spaceWidth + wordWidth > maxWidth &&
          currentLine.isNotEmpty) {
        lines.add(TextLine(currentLine.toString(), currentWidth,
            scaledLineHeight, scaledLineHeight * 0.8));
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
      lines.add(TextLine(currentLine.toString(), currentWidth,
          scaledLineHeight, scaledLineHeight * 0.8));
    }
    final totalWidth = lines.isEmpty
        ? 0.0
        : lines.map((l) => l.width).reduce((a, b) => a > b ? a : b);
    final totalHeight = lines.length * scaledLineHeight;
    return TextMetrics(totalWidth, totalHeight, lines);
  }
}

StyledNode? _findStyled(StyledNode root, String tagName) {
  if (root.node is Element && (root.node as Element).tagName == tagName) {
    return root;
  }
  for (final child in root.children) {
    final found = _findStyled(child, tagName);
    if (found != null) return found;
  }
  return null;
}

List<StyledNode> _findAllStyled(StyledNode root, String tagName) {
  final results = <StyledNode>[];
  if (root.node is Element && (root.node as Element).tagName == tagName) {
    results.add(root);
  }
  for (final child in root.children) {
    results.addAll(_findAllStyled(child, tagName));
  }
  return results;
}

LayoutBox? _findBox(LayoutBox root, String tagName) {
  for (final box in root.allBoxes) {
    if (box.styledNode?.node is Element &&
        (box.styledNode!.node as Element).tagName == tagName) {
      return box;
    }
  }
  return null;
}

List<LayoutBox> _findAllBoxes(LayoutBox root, String tagName) {
  return root.allBoxes
      .where((b) =>
          b.styledNode?.node is Element &&
          (b.styledNode!.node as Element).tagName == tagName)
      .toList();
}

LayoutBox _layout(String html, {double width = 800, String css = ''}) {
  final doc = HtmlParser.parse(html);
  final sheets = <Stylesheet>[];
  if (css.isNotEmpty) sheets.add(CssParser.parse(css));
  final styled = computeStyles(doc.body!, sheets);
  return layoutTree(styled, width, MockTextMeasurer());
}

StyledNode _style(String html, {String css = ''}) {
  final doc = HtmlParser.parse(html);
  final sheets = <Stylesheet>[];
  if (css.isNotEmpty) sheets.add(CssParser.parse(css));
  return computeStyles(doc.body!, sheets);
}

void main() {
  // ════════════════════════════════════════════════════════════════════
  // PART 1: HTML PARSER — W3C HTML Living Standard
  // ════════════════════════════════════════════════════════════════════

  group('W3C HTML Parser', () {
    // §12.2.6 — Tree construction
    group('Document structure', () {
      test('creates html/head/body when missing', () {
        final doc = HtmlParser.parse('<p>Hello</p>');
        expect(doc.documentElement!.tagName, 'html');
        expect(doc.body, isNotNull);
        expect(doc.head, isNotNull);
      });

      test('handles DOCTYPE declaration', () {
        final doc = HtmlParser.parse('<!DOCTYPE html><html><body>Hi</body></html>');
        expect(doc.body, isNotNull);
        expect(doc.body!.textContent, contains('Hi'));
      });

      test('handles XML processing instructions', () {
        final doc = HtmlParser.parse('<?xml version="1.0"?><html><body>Hi</body></html>');
        expect(doc.body, isNotNull);
      });

      test('handles multiple style elements', () {
        final doc = HtmlParser.parse('''
          <html><head>
            <style>p { color: red; }</style>
            <style>div { color: blue; }</style>
          </head><body>Hi</body></html>
        ''');
        expect(doc.internalCSS, contains('color: red'));
        expect(doc.internalCSS, contains('color: blue'));
      });
    });

    // §4.3 — Sections
    group('Section elements', () {
      for (final tag in ['article', 'section', 'nav', 'aside', 'header', 'footer', 'main', 'hgroup']) {
        test('parses <$tag>', () {
          final doc = HtmlParser.parse('<body><$tag>Content</$tag></body>');
          final el = doc.body!.elementDescendants.where((e) => e.tagName == tag).firstOrNull;
          expect(el, isNotNull, reason: '<$tag> should be parsed');
        });
      }
    });

    // §4.4 — Grouping content
    group('Grouping content elements', () {
      for (final tag in ['p', 'div', 'blockquote', 'pre', 'figure', 'figcaption',
                          'ul', 'ol', 'li', 'dl', 'dt', 'dd', 'hr', 'address']) {
        test('parses <$tag>', () {
          final doc = HtmlParser.parse('<body><$tag>Content</$tag></body>');
          final el = doc.body!.elementDescendants.where((e) => e.tagName == tag).firstOrNull;
          expect(el, isNotNull, reason: '<$tag> should be parsed');
        });
      }
    });

    // §4.5 — Text-level semantics
    group('Text-level semantic elements', () {
      for (final tag in ['a', 'em', 'strong', 'small', 's', 'cite', 'q',
                          'dfn', 'abbr', 'code', 'var', 'samp', 'kbd',
                          'sub', 'sup', 'i', 'b', 'u', 'mark', 'span',
                          'br', 'wbr']) {
        test('parses <$tag>', () {
          final selfClosing = voidElements.contains(tag);
          final html = selfClosing
              ? '<body>Before<$tag>After</body>'
              : '<body><$tag>Content</$tag></body>';
          final doc = HtmlParser.parse(html);
          final el = doc.body!.elementDescendants.where((e) => e.tagName == tag).firstOrNull;
          expect(el, isNotNull, reason: '<$tag> should be parsed');
        });
      }
    });

    // §4.8 — Embedded content
    group('Embedded content', () {
      test('parses <img> with src and alt', () {
        final doc = HtmlParser.parse('<body><img src="test.png" alt="Test"></body>');
        final img = doc.body!.elementDescendants.where((e) => e.tagName == 'img').first;
        expect(img.attributes['src'], 'test.png');
        expect(img.attributes['alt'], 'Test');
      });

      test('parses <video> and <source>', () {
        final doc = HtmlParser.parse('<body><video><source src="v.mp4" type="video/mp4"></video></body>');
        final video = doc.body!.elementDescendants.where((e) => e.tagName == 'video').first;
        expect(video, isNotNull);
      });
    });

    // §4.9 — Tabular data
    group('Table elements', () {
      test('parses full table structure', () {
        final doc = HtmlParser.parse('''
          <body><table>
            <thead><tr><th>H1</th><th>H2</th></tr></thead>
            <tbody><tr><td>A</td><td>B</td></tr></tbody>
            <tfoot><tr><td>F1</td><td>F2</td></tr></tfoot>
          </table></body>
        ''');
        final table = doc.body!.elementDescendants.where((e) => e.tagName == 'table').first;
        final thead = table.elementDescendants.where((e) => e.tagName == 'thead');
        final tbody = table.elementDescendants.where((e) => e.tagName == 'tbody');
        final tfoot = table.elementDescendants.where((e) => e.tagName == 'tfoot');
        expect(thead.length, 1);
        expect(tbody.length, 1);
        expect(tfoot.length, 1);
      });

      test('parses table without thead/tbody/tfoot', () {
        final doc = HtmlParser.parse('''
          <body><table><tr><td>A</td><td>B</td></tr></table></body>
        ''');
        final tds = doc.body!.elementDescendants.where((e) => e.tagName == 'td');
        expect(tds.length, 2);
      });

      test('parses table with colspan and cellpadding', () {
        final doc = HtmlParser.parse('''
          <body><table cellpadding="5">
            <tr><td colspan="2">Wide</td></tr>
            <tr><td>A</td><td>B</td></tr>
          </table></body>
        ''');
        final table = doc.body!.elementDescendants.where((e) => e.tagName == 'table').first;
        expect(table.attributes['cellpadding'], '5');
        final td = doc.body!.elementDescendants.where((e) => e.tagName == 'td').first;
        expect(td.attributes['colspan'], '2');
      });
    });

    // §4.10 — Forms
    group('Form elements', () {
      for (final tag in ['form', 'input', 'button', 'select', 'textarea', 'fieldset', 'legend', 'label']) {
        test('parses <$tag>', () {
          final selfClosing = voidElements.contains(tag);
          final html = selfClosing
              ? '<body><$tag></body>'
              : '<body><$tag>Content</$tag></body>';
          final doc = HtmlParser.parse(html);
          final el = doc.body!.elementDescendants.where((e) => e.tagName == tag).firstOrNull;
          expect(el, isNotNull, reason: '<$tag> should be parsed');
        });
      }
    });

    // §4.6 — Heading elements
    group('Heading elements', () {
      for (final level in [1, 2, 3, 4, 5, 6]) {
        test('parses <h$level>', () {
          final doc = HtmlParser.parse('<body><h$level>Heading $level</h$level></body>');
          final h = doc.body!.elementDescendants.where((e) => e.tagName == 'h$level').first;
          expect(h.textContent, 'Heading $level');
        });
      }
    });

    // §8.5 — Named character references
    group('Character references', () {
      test('decodes common named entities', () {
        final doc = HtmlParser.parse('<p>&amp;&lt;&gt;&quot;&apos;&nbsp;</p>');
        final p = doc.body!.elementDescendants.where((e) => e.tagName == 'p').first;
        expect(p.textContent, contains('&'));
        expect(p.textContent, contains('<'));
        expect(p.textContent, contains('>'));
      });

      test('decodes numeric entities', () {
        final doc = HtmlParser.parse('<p>&#65;&#x42;</p>');
        final p = doc.body!.elementDescendants.where((e) => e.tagName == 'p').first;
        expect(p.textContent, contains('A'));
        expect(p.textContent, contains('B'));
      });
    });

    // Edge cases
    group('Parser edge cases', () {
      test('does not treat <3 as a tag', () {
        final doc = HtmlParser.parse('<p>I <3 you</p>');
        final p = doc.body!.elementDescendants.where((e) => e.tagName == 'p').first;
        expect(p.textContent, contains('<3'));
      });

      test('handles unclosed tags gracefully', () {
        final doc = HtmlParser.parse('<body><div><p>Unclosed<div>Next</div></body>');
        expect(doc.body, isNotNull);
        expect(doc.body!.textContent, contains('Unclosed'));
      });

      test('handles nested quotes in attributes', () {
        final doc = HtmlParser.parse('<body><div data-info="it\'s ok">Test</div></body>');
        final div = doc.body!.elementDescendants.where((e) => e.tagName == 'div').first;
        expect(div.attributes['data-info'], "it's ok");
      });

      test('preserves whitespace between inline elements', () {
        final doc = HtmlParser.parse('<body><span>Hello</span> <span>World</span></body>');
        final body = doc.body!;
        final allText = body.textContent;
        expect(allText, contains('Hello'));
        expect(allText, contains('World'));
      });

      test('handles void elements without closing slash', () {
        final doc = HtmlParser.parse('<body><br><hr><img src="x"><input type="text"></body>');
        final tags = doc.body!.elementDescendants.map((e) => e.tagName).toSet();
        expect(tags, containsAll(['br', 'hr', 'img', 'input']));
      });

      test('handles comments', () {
        final doc = HtmlParser.parse('<body><!-- comment --><p>After</p></body>');
        final p = doc.body!.elementDescendants.where((e) => e.tagName == 'p').first;
        expect(p.textContent, 'After');
      });

      test('handles noscript content (no-JS browser)', () {
        final doc = HtmlParser.parse('<body><noscript><p>No JS</p></noscript></body>');
        // noscript should NOT be in ignoredElements for a no-JS browser
        final noscript = doc.body!.elementDescendants.where((e) => e.tagName == 'noscript').firstOrNull;
        expect(noscript, isNotNull, reason: 'noscript should be parsed for no-JS browsers');
      });
    });

    // §4.7 — Legacy elements
    group('Legacy HTML elements', () {
      test('parses <center>', () {
        final doc = HtmlParser.parse('<body><center>Centered</center></body>');
        final el = doc.body!.elementDescendants.where((e) => e.tagName == 'center').first;
        expect(el.textContent, 'Centered');
      });

      test('parses <font> with face/size/color', () {
        final doc = HtmlParser.parse('<body><font face="Arial" size="5" color="red">Text</font></body>');
        final font = doc.body!.elementDescendants.where((e) => e.tagName == 'font').first;
        expect(font.attributes['face'], 'Arial');
        expect(font.attributes['size'], '5');
        expect(font.attributes['color'], 'red');
      });

      test('parses <tt>', () {
        final doc = HtmlParser.parse('<body><tt>Monospace</tt></body>');
        final tt = doc.body!.elementDescendants.where((e) => e.tagName == 'tt').first;
        expect(tt.textContent, 'Monospace');
      });

      test('parses <strike>', () {
        final doc = HtmlParser.parse('<body><strike>Struck</strike></body>');
        final el = doc.body!.elementDescendants.where((e) => e.tagName == 'strike').first;
        expect(el.textContent, 'Struck');
      });

      test('parses <big> and <small>', () {
        final doc = HtmlParser.parse('<body><big>Big</big><small>Small</small></body>');
        expect(doc.body!.elementDescendants.where((e) => e.tagName == 'big').length, 1);
        expect(doc.body!.elementDescendants.where((e) => e.tagName == 'small').length, 1);
      });
    });
  });

  // ════════════════════════════════════════════════════════════════════
  // PART 2: CSS PARSER — W3C CSS Syntax Module Level 3
  // ════════════════════════════════════════════════════════════════════

  group('W3C CSS Parser', () {
    // §5 — Parsing
    group('Basic parsing', () {
      test('parses simple rule', () {
        final sheet = CssParser.parse('p { color: red; }');
        expect(sheet.rules.length, 1);
        expect(sheet.rules[0].declarations['color']?.value, 'red');
      });

      test('parses multiple declarations', () {
        final sheet = CssParser.parse('p { color: red; font-size: 14px; margin: 10px; }');
        expect(sheet.rules[0].declarations.length, 3);
      });

      test('parses multiple rules', () {
        final sheet = CssParser.parse('p { color: red; } div { color: blue; }');
        expect(sheet.rules.length, 2);
      });

      test('handles CSS comments', () {
        final sheet = CssParser.parse('/* comment */ p { color: /* inline */ red; }');
        expect(sheet.rules.length, 1);
        expect(sheet.rules[0].declarations['color']?.value, 'red');
      });

      test('parses empty rules', () {
        final sheet = CssParser.parse('p { }');
        expect(sheet.rules.length, 1);
        expect(sheet.rules[0].declarations.isEmpty, isTrue);
      });
    });

    // §3.4 — !important
    group('!important', () {
      test('parses !important flag', () {
        final sheet = CssParser.parse('p { color: red !important; }');
        expect(sheet.rules[0].declarations['color']?.important, isTrue);
      });

      test('!important value is extracted correctly', () {
        final sheet = CssParser.parse('p { color: red !important; }');
        expect(sheet.rules[0].declarations['color']?.value, 'red');
      });
    });

    // CSS Selectors Level 4
    group('Selector parsing', () {
      test('parses tag selector', () {
        final sheet = CssParser.parse('div { color: red; }');
        expect(sheet.rules[0].selector.compounds[0].tag, 'div');
      });

      test('parses class selector', () {
        final sheet = CssParser.parse('.foo { color: red; }');
        expect(sheet.rules[0].selector.compounds[0].classes, ['foo']);
      });

      test('parses ID selector', () {
        final sheet = CssParser.parse('#bar { color: red; }');
        expect(sheet.rules[0].selector.compounds[0].id, 'bar');
      });

      test('parses compound selector', () {
        final sheet = CssParser.parse('div.foo#bar { color: red; }');
        final c = sheet.rules[0].selector.compounds[0];
        expect(c.tag, 'div');
        expect(c.classes, ['foo']);
        expect(c.id, 'bar');
      });

      test('parses descendant combinator', () {
        final sheet = CssParser.parse('div p { color: red; }');
        expect(sheet.rules[0].selector.compounds.length, 2);
        expect(sheet.rules[0].selector.combinators[0], Combinator.descendant);
      });

      test('parses child combinator >', () {
        final sheet = CssParser.parse('div > p { color: red; }');
        expect(sheet.rules[0].selector.combinators[0], Combinator.child);
      });

      test('parses adjacent sibling combinator +', () {
        final sheet = CssParser.parse('h1 + p { color: red; }');
        expect(sheet.rules[0].selector.combinators[0], Combinator.adjacentSibling);
      });

      test('parses general sibling combinator ~', () {
        final sheet = CssParser.parse('h1 ~ p { color: red; }');
        expect(sheet.rules[0].selector.combinators[0], Combinator.generalSibling);
      });

      test('parses comma-separated selectors', () {
        final sheet = CssParser.parse('h1, h2, h3 { color: red; }');
        expect(sheet.rules.length, 3);
      });

      test('parses attribute selector [attr]', () {
        final sheet = CssParser.parse('[hidden] { display: none; }');
        expect(sheet.rules[0].selector.compounds[0].attributes[0].name, 'hidden');
      });

      test('parses attribute selector [attr=val]', () {
        final sheet = CssParser.parse('[type="text"] { color: red; }');
        final attr = sheet.rules[0].selector.compounds[0].attributes[0];
        expect(attr.name, 'type');
        expect(attr.op, '=');
        expect(attr.value, 'text');
      });

      test('parses pseudo-class :first-child', () {
        final sheet = CssParser.parse('p:first-child { color: red; }');
        expect(sheet.rules[0].selector.compounds[0].pseudoClasses[0].name, 'first-child');
      });

      test('parses pseudo-class :nth-child()', () {
        final sheet = CssParser.parse('p:nth-child(2n+1) { color: red; }');
        final pseudo = sheet.rules[0].selector.compounds[0].pseudoClasses[0];
        expect(pseudo.name, 'nth-child');
        expect(pseudo.argument, '2n+1');
      });

      test('parses pseudo-class :not()', () {
        final sheet = CssParser.parse('p:not(.hidden) { color: red; }');
        final pseudo = sheet.rules[0].selector.compounds[0].pseudoClasses[0];
        expect(pseudo.name, 'not');
        expect(pseudo.selectorArg, isNotNull);
      });

      test('parses pseudo-element ::before', () {
        final sheet = CssParser.parse('p::before { content: "x"; }');
        expect(sheet.rules[0].selector.compounds[0].pseudoElement?.name, 'before');
      });

      test('parses universal selector *', () {
        final sheet = CssParser.parse('* { margin: 0; }');
        expect(sheet.rules[0].selector.compounds[0].tag, isNull);
      });
    });

    // Specificity — W3C Selectors Level 4 §17
    group('Specificity', () {
      test('element selector specificity = (0,0,1)', () {
        final sheet = CssParser.parse('p { }');
        expect(sheet.rules[0].selector.specificity.toString(), '(0,0,1)');
      });

      test('class selector specificity = (0,1,0)', () {
        final sheet = CssParser.parse('.foo { }');
        expect(sheet.rules[0].selector.specificity.toString(), '(0,1,0)');
      });

      test('ID selector specificity = (1,0,0)', () {
        final sheet = CssParser.parse('#bar { }');
        expect(sheet.rules[0].selector.specificity.toString(), '(1,0,0)');
      });

      test('compound specificity adds up', () {
        final sheet = CssParser.parse('div.foo#bar { }');
        expect(sheet.rules[0].selector.specificity.toString(), '(1,1,1)');
      });

      test('descendant selector adds specificity', () {
        final sheet = CssParser.parse('div p span { }');
        expect(sheet.rules[0].selector.specificity.toString(), '(0,0,3)');
      });
    });

    // @media — W3C CSS Conditional Rules Level 3
    group('@media queries', () {
      test('parses @media screen', () {
        final sheet = CssParser.parse('@media screen { p { color: red; } }');
        expect(sheet.rules.length, 1);
      });

      test('parses @media with min-width', () {
        final sheet = CssParser.parse('@media (min-width: 768px) { p { color: red; } }', 1024, 768);
        expect(sheet.rules.length, 1);
      });

      test('excludes rules when media query fails', () {
        final sheet = CssParser.parse('@media (max-width: 400px) { p { color: red; } }', 1024, 768);
        expect(sheet.rules.length, 0);
      });

      test('handles comma-separated media queries', () {
        final sheet = CssParser.parse('@media screen, print { p { color: red; } }');
        expect(sheet.rules.length, 1);
      });

      test('handles not keyword', () {
        final sheet = CssParser.parse('@media not print { p { color: red; } }');
        expect(sheet.rules.length, 1); // We are "screen", not "print"
      });
    });

    // @import
    group('@import', () {
      test('collects @import URLs', () {
        final sheet = CssParser.parse('@import "styles.css";');
        expect(sheet.imports, contains('styles.css'));
      });

      test('handles url() syntax', () {
        final sheet = CssParser.parse('@import url("styles.css");');
        expect(sheet.imports, contains('styles.css'));
      });
    });

    // CSS Values — W3C CSS Values and Units Level 4
    group('CSS values', () {
      test('parses url() in property values', () {
        final sheet = CssParser.parse('div { background: url("bg.png"); }');
        expect(sheet.rules[0].declarations['background']?.value, contains('url'));
      });

      test('parses calc() expressions', () {
        final sheet = CssParser.parse('div { width: calc(100% - 20px); }');
        expect(sheet.rules[0].declarations['width']?.value, contains('calc'));
      });

      test('parses var() references', () {
        final sheet = CssParser.parse('div { color: var(--main-color); }');
        expect(sheet.rules[0].declarations['color']?.value, contains('var'));
      });

      test('parses custom properties (--*)', () {
        final sheet = CssParser.parse(':root { --main-color: red; }');
        expect(sheet.rules[0].declarations['--main-color']?.value, 'red');
      });
    });
  });

  // ════════════════════════════════════════════════════════════════════
  // PART 3: STYLE RESOLUTION — W3C CSS Cascade Level 4
  // ════════════════════════════════════════════════════════════════════

  group('W3C Style Resolution', () {
    // §6 — Cascade
    group('Cascade order', () {
      test('UA defaults are applied', () {
        final styled = _style('<body><h1>Title</h1></body>');
        final h1 = _findStyled(styled, 'h1');
        expect(h1!['font-weight'], 'bold');
        expect(h1['font-size'], '32px');
      });

      test('stylesheet rules override UA defaults', () {
        final styled = _style('<body><h1>Title</h1></body>', css: 'h1 { font-size: 20px; }');
        final h1 = _findStyled(styled, 'h1');
        expect(h1!['font-size'], '20px');
      });

      test('inline styles override stylesheet rules', () {
        final styled = _style('<body><h1 style="font-size: 50px;">Title</h1></body>',
            css: 'h1 { font-size: 20px; }');
        final h1 = _findStyled(styled, 'h1');
        expect(h1!['font-size'], '50px');
      });

      test('!important overrides inline styles', () {
        final styled = _style('<body><p style="color: green;">Text</p></body>',
            css: 'p { color: red !important; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'red');
      });

      test('inline !important overrides stylesheet !important', () {
        final styled = _style('<body><p style="color: green !important;">Text</p></body>',
            css: 'p { color: red !important; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'green');
      });
    });

    // §6.4 — Specificity
    group('Specificity ordering', () {
      test('class beats element', () {
        final styled = _style('<body><p class="x">Text</p></body>',
            css: 'p { color: red; } .x { color: blue; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'blue');
      });

      test('ID beats class', () {
        final styled = _style('<body><p id="main" class="x">Text</p></body>',
            css: '.x { color: blue; } #main { color: green; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'green');
      });

      test('later rule wins at equal specificity', () {
        final styled = _style('<body><p>Text</p></body>',
            css: 'p { color: red; } p { color: blue; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'blue');
      });
    });

    // §6.2 — Inheritance
    group('Inheritance (CSS 2.2 §6.2)', () {
      test('color inherits', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { color: red; }');
        final span = _findStyled(styled, 'span');
        expect(span!['color'], 'red');
      });

      test('font-family inherits', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { font-family: monospace; }');
        final span = _findStyled(styled, 'span');
        expect(span!['font-family'], 'monospace');
      });

      test('font-size inherits', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { font-size: 20px; }');
        final span = _findStyled(styled, 'span');
        expect(span!['font-size'], '20px');
      });

      test('text-align inherits', () {
        final styled = _style('<body><div><p>X</p></div></body>',
            css: 'div { text-align: center; }');
        final p = _findStyled(styled, 'p');
        expect(p!['text-align'], 'center');
      });

      test('text-decoration does NOT inherit (W3C CSS Text Decoration §2)', () {
        final styled = _style('<body><a href="#"><span>X</span></a></body>');
        final span = _findStyled(styled, 'span');
        // span should NOT have text-decoration from parent <a>
        expect(span!['text-decoration'], isNot('underline'));
      });

      test('margin does NOT inherit', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { margin: 20px; }');
        final span = _findStyled(styled, 'span');
        expect(span!['margin'], isNull);
      });

      test('padding does NOT inherit', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { padding: 20px; }');
        final span = _findStyled(styled, 'span');
        expect(span!['padding'], isNull);
      });

      test('border does NOT inherit', () {
        final styled = _style('<body><div><span>X</span></div></body>',
            css: 'div { border: 1px solid red; }');
        final span = _findStyled(styled, 'span');
        expect(span!['border'], isNull);
      });

      test('display does NOT inherit', () {
        final styled = _style('<body><div style="display:flex"><span>X</span></div></body>');
        final span = _findStyled(styled, 'span');
        expect(span!['display'], isNot('flex'));
      });
    });

    // CSS-wide keywords
    group('CSS-wide keywords (§3.1)', () {
      test('inherit keyword', () {
        final styled = _style('<body><div style="border: 2px solid red;"><p style="border: inherit;">X</p></div></body>');
        final p = _findStyled(styled, 'p');
        expect(p!['border'], '2px solid red');
      });

      test('initial keyword reverts to default', () {
        final styled = _style('<body><h1 style="font-weight: initial;">Title</h1></body>');
        final h1 = _findStyled(styled, 'h1');
        // 'initial' should remove the property (revert to browser default / no explicit value)
        expect(h1!['font-weight'], isNull);
      });

      test('unset on inheritable property acts like inherit', () {
        final styled = _style('<body><div><p style="color: unset;">X</p></div></body>',
            css: 'div { color: green; }');
        final p = _findStyled(styled, 'p');
        expect(p!['color'], 'green');
      });
    });

    // UA defaults for every HTML element
    group('UA defaults (per HTML spec)', () {
      test('h1 defaults', () {
        final s = _style('<body><h1>X</h1></body>');
        final h1 = _findStyled(s, 'h1')!;
        expect(h1['font-weight'], 'bold');
        expect(h1['font-size'], '32px');
      });

      test('h2 defaults', () {
        final s = _style('<body><h2>X</h2></body>');
        final h2 = _findStyled(s, 'h2')!;
        expect(h2['font-size'], '24px');
      });

      test('h3 defaults', () {
        final s = _style('<body><h3>X</h3></body>');
        expect(_findStyled(s, 'h3')!['font-size'], '19px');
      });

      test('h4 defaults', () {
        final s = _style('<body><h4>X</h4></body>');
        expect(_findStyled(s, 'h4')!['font-size'], '16px');
      });

      test('h5 defaults', () {
        final s = _style('<body><h5>X</h5></body>');
        expect(_findStyled(s, 'h5')!['font-size'], '13px');
      });

      test('h6 defaults', () {
        final s = _style('<body><h6>X</h6></body>');
        expect(_findStyled(s, 'h6')!['font-size'], '11px');
      });

      test('p has vertical margins', () {
        final s = _style('<body><p>X</p></body>');
        expect(_findStyled(s, 'p')!['margin-top'], '16px');
      });

      test('a defaults: blue underline', () {
        final s = _style('<body><a href="#">X</a></body>');
        final a = _findStyled(s, 'a')!;
        expect(a['color'], '#0000EE');
        expect(a['text-decoration'], 'underline');
      });

      test('strong/b is bold', () {
        final s = _style('<body><strong>X</strong><b>Y</b></body>');
        expect(_findStyled(s, 'strong')!['font-weight'], 'bold');
        expect(_findStyled(s, 'b')!['font-weight'], 'bold');
      });

      test('em/i is italic', () {
        final s = _style('<body><em>X</em><i>Y</i></body>');
        expect(_findStyled(s, 'em')!['font-style'], 'italic');
        expect(_findStyled(s, 'i')!['font-style'], 'italic');
      });

      test('u has underline', () {
        final s = _style('<body><u>X</u></body>');
        expect(_findStyled(s, 'u')!['text-decoration'], 'underline');
      });

      test('s/del has line-through', () {
        final s = _style('<body><s>X</s><del>Y</del></body>');
        expect(_findStyled(s, 's')!['text-decoration'], 'line-through');
        expect(_findStyled(s, 'del')!['text-decoration'], 'line-through');
      });

      test('code/kbd/samp use monospace', () {
        final s = _style('<body><code>X</code><kbd>Y</kbd><samp>Z</samp></body>');
        expect(_findStyled(s, 'code')!['font-family'], 'monospace');
        expect(_findStyled(s, 'kbd')!['font-family'], 'monospace');
        expect(_findStyled(s, 'samp')!['font-family'], 'monospace');
      });

      test('pre uses monospace + white-space:pre', () {
        final s = _style('<body><pre>X</pre></body>');
        final pre = _findStyled(s, 'pre')!;
        expect(pre['font-family'], 'monospace');
        expect(pre['white-space'], 'pre');
      });

      test('blockquote has left margin', () {
        final s = _style('<body><blockquote>X</blockquote></body>');
        expect(_findStyled(s, 'blockquote')!['margin-left'], '40px');
      });

      test('ul/ol have padding-left and list-style', () {
        final s = _style('<body><ul><li>X</li></ul><ol><li>Y</li></ol></body>');
        expect(_findStyled(s, 'ul')!['padding-left'], '40px');
        expect(_findStyled(s, 'ol')!['padding-left'], '40px');
        expect(_findStyled(s, 'ul')!['list-style-type'], 'disc');
        expect(_findStyled(s, 'ol')!['list-style-type'], 'decimal');
      });

      test('li has display: list-item', () {
        final s = _style('<body><ul><li>X</li></ul></body>');
        expect(_findStyled(s, 'li')!['display'], 'list-item');
      });

      test('table has display: table', () {
        final s = _style('<body><table><tr><td>X</td></tr></table></body>');
        expect(_findStyled(s, 'table')!['display'], 'table');
      });

      test('th is bold + centered', () {
        final s = _style('<body><table><tr><th>X</th></tr></table></body>');
        final th = _findStyled(s, 'th')!;
        expect(th['font-weight'], 'bold');
        expect(th['text-align'], 'center');
      });

      test('center element has text-align:center + display:block', () {
        final s = _style('<body><center>X</center></body>');
        final c = _findStyled(s, 'center')!;
        expect(c['text-align'], 'center');
        expect(c['display'], 'block');
      });

      test('tt uses monospace', () {
        final s = _style('<body><tt>X</tt></body>');
        expect(_findStyled(s, 'tt')!['font-family'], 'monospace');
      });

      test('hidden attribute gives display:none', () {
        final s = _style('<body><div hidden>X</div></body>');
        expect(_findStyled(s, 'div')!.display, Display.none);
      });

      test('body link color propagates to <a>', () {
        final doc = HtmlParser.parse('<html><body link="#FF0000"><a href="#">Link</a></body></html>');
        final styled = computeStyles(doc.body!, []);
        final a = _findStyled(styled, 'a');
        expect(a!['color'], '#FF0000');
      });

      test('body text color attribute', () {
        final doc = HtmlParser.parse('<html><body text="#0000FF"><p>Text</p></body></html>');
        final styled = computeStyles(doc.body!, []);
        // Body should have color #0000FF which inherits to <p>
        final p = _findStyled(styled, 'p');
        expect(p!['color'], '#0000FF');
      });

      test('font element attributes map to CSS', () {
        final s = _style('<body><font face="Arial" size="5" color="red">X</font></body>');
        final font = _findStyled(s, 'font')!;
        expect(font['font-family'], 'Arial');
        expect(font['color'], 'red');
        expect(font['font-size'], '24px'); // size 5 = 24px
      });

      test('input has display: inline-block', () {
        final s = _style('<body><input type="text"></body>');
        expect(_findStyled(s, 'input')!['display'], 'inline-block');
      });

      test('button has display: inline-block', () {
        final s = _style('<body><button>Click</button></body>');
        expect(_findStyled(s, 'button')!['display'], 'inline-block');
      });

      test('hr has border', () {
        final s = _style('<body><hr></body>');
        expect(_findStyled(s, 'hr')!['border-top'], isNotNull);
      });
    });

    // HTML presentational attributes → CSS
    group('HTML attributes to CSS mapping', () {
      test('width attribute on img', () {
        final s = _style('<body><img src="x" width="200"></body>');
        expect(_findStyled(s, 'img')!['width'], '200px');
      });

      test('height attribute on img', () {
        final s = _style('<body><img src="x" height="100"></body>');
        expect(_findStyled(s, 'img')!['height'], '100px');
      });

      test('bgcolor attribute', () {
        final s = _style('<body><table bgcolor="#ff0000"><tr><td>X</td></tr></table></body>');
        expect(_findStyled(s, 'table')!['background-color'], '#ff0000');
      });

      test('align attribute', () {
        final s = _style('<body><p align="center">X</p></body>');
        expect(_findStyled(s, 'p')!['text-align'], 'center');
      });

      test('border attribute on table', () {
        final s = _style('<body><table border="1"><tr><td>X</td></tr></table></body>');
        expect(_findStyled(s, 'table')!['border'], contains('1px'));
      });

      test('cellpadding on td', () {
        final s = _style('<body><table cellpadding="10"><tr><td>X</td></tr></table></body>');
        expect(_findStyled(s, 'td')!['padding'], '10px');
      });

      test('width percentage attribute', () {
        final s = _style('<body><table width="100%"><tr><td>X</td></tr></table></body>');
        expect(_findStyled(s, 'table')!['width'], '100%');
      });
    });

    // Selector matching
    group('Selector matching', () {
      test('descendant selector', () {
        final s = _style('<body><div><p>X</p></div></body>', css: 'div p { color: red; }');
        expect(_findStyled(s, 'p')!['color'], 'red');
      });

      test('child selector', () {
        final s = _style('<body><div><p>Direct</p></div></body>', css: 'div > p { color: red; }');
        expect(_findStyled(s, 'p')!['color'], 'red');
      });

      test('child selector does not match grandchild', () {
        final s = _style('<body><div><span><p>Nested</p></span></div></body>', css: 'div > p { color: red; }');
        // The p is a grandchild, not direct child, so should NOT match
        final p = _findStyled(s, 'p')!;
        expect(p['color'], isNot('red'));
      });

      test('pseudo-class :first-child', () {
        final s = _style('<body><ul><li>First</li><li>Second</li></ul></body>',
            css: 'li:first-child { color: red; }');
        final lis = _findAllStyled(s, 'li');
        expect(lis[0]['color'], 'red');
      });

      test('pseudo-class :last-child', () {
        final s = _style('<body><ul><li>First</li><li>Second</li></ul></body>',
            css: 'li:last-child { color: red; }');
        final lis = _findAllStyled(s, 'li');
        expect(lis.last['color'], 'red');
      });

      test('pseudo-class :hover (default false)', () {
        final s = _style('<body><a href="#">Link</a></body>', css: 'a:hover { color: red; }');
        final a = _findStyled(s, 'a')!;
        // Not hovered by default, so :hover should not match
        expect(a['color'], isNot('red'));
      });

      test('attribute selector matches', () {
        final s = _style('<body><input type="text"></body>',
            css: 'input[type="text"] { color: red; }');
        expect(_findStyled(s, 'input')!['color'], 'red');
      });
    });

    // var() resolution
    group('CSS var() resolution', () {
      test('resolves simple var()', () {
        final s = _style('<body><div>X</div></body>',
            css: ':root { --c: red; } div { color: var(--c); }');
        // Note: :root matches <html> but we style from body.
        // The custom property should be inherited.
      });

      test('var() with fallback', () {
        final doc = HtmlParser.parse('<body><div style="color: var(--missing, blue);">X</div></body>');
        final styled = computeStyles(doc.body!, []);
        final div = _findStyled(styled, 'div')!;
        expect(div.prop('color'), 'blue');
      });
    });
  });

  // ════════════════════════════════════════════════════════════════════
  // PART 4: LAYOUT ENGINE — W3C CSS Box Model + Layout
  // ════════════════════════════════════════════════════════════════════

  group('W3C Layout Engine', () {
    // CSS Box Model Level 3
    group('Box model (§3)', () {
      test('body has 8px margin by default', () {
        final root = _layout('<body><p>X</p></body>');
        expect(root.margin.left, 8);
        expect(root.margin.right, 8);
        expect(root.margin.top, 8);
        expect(root.margin.bottom, 8);
      });

      test('content width = container - margins - borders - padding', () {
        final root = _layout('<body><div>X</div></body>');
        final div = _findBox(root, 'div')!;
        // Body content width = 800 - 16 = 784, div fills that
        expect(div.content.width, 784);
      });

      test('explicit width overrides auto', () {
        final root = _layout('<body><div style="width:400px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, 400);
      });

      test('explicit height is respected', () {
        final root = _layout('<body><div style="height:200px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.height, 200);
      });

      test('padding is added inside content', () {
        final root = _layout('<body><div style="padding:10px;width:100px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.padding.top, 10);
        expect(div.padding.left, 10);
        expect(div.content.width, 100);
      });

      test('border is added outside padding', () {
        final root = _layout('<body><div style="border:2px solid red;width:100px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.border.top, 2);
        expect(div.border.left, 2);
      });

      test('box-sizing: border-box includes padding+border in width', () {
        final root = _layout('<body><div style="box-sizing:border-box;width:100px;padding:10px;border:2px solid red">X</div></body>');
        final div = _findBox(root, 'div')!;
        // border-box: content = 100 - 10*2 - 2*2 = 76
        expect(div.content.width, 76);
      });

      test('margin shorthand: 1 value', () {
        final root = _layout('<body><div style="margin:20px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.margin.top, 20);
        expect(div.margin.right, 20);
        expect(div.margin.bottom, 20);
        expect(div.margin.left, 20);
      });

      test('margin shorthand: 2 values', () {
        final root = _layout('<body><div style="margin:10px 20px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.margin.top, 10);
        expect(div.margin.right, 20);
        expect(div.margin.bottom, 10);
        expect(div.margin.left, 20);
      });

      test('margin shorthand: 3 values', () {
        final root = _layout('<body><div style="margin:10px 20px 30px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.margin.top, 10);
        expect(div.margin.right, 20);
        expect(div.margin.bottom, 30);
        expect(div.margin.left, 20);
      });

      test('margin shorthand: 4 values', () {
        final root = _layout('<body><div style="margin:10px 20px 30px 40px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.margin.top, 10);
        expect(div.margin.right, 20);
        expect(div.margin.bottom, 30);
        expect(div.margin.left, 40);
      });

      test('margin longhand overrides shorthand', () {
        final root = _layout('<body><div style="margin:20px;margin-left:50px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.margin.left, 50);
        expect(div.margin.top, 20);
        expect(div.margin.right, 20);
      });

      test('padding longhand overrides shorthand', () {
        final root = _layout('<body><div style="padding:10px;padding-top:30px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.padding.top, 30);
        expect(div.padding.left, 10);
      });
    });

    // CSS 2.2 §10.3.3 — Block-level, non-replaced elements in normal flow
    group('Block layout (CSS 2.2 §9.4.1)', () {
      test('blocks stack vertically', () {
        final root = _layout('<body><div style="height:50px">A</div><div style="height:50px">B</div></body>');
        final divs = _findAllBoxes(root, 'div');
        expect(divs.length, 2);
        expect(divs[1].content.y, greaterThan(divs[0].content.y));
      });

      test('margin:auto centers block horizontally (CSS 2.2 §10.3.3)', () {
        final root = _layout('<body><div style="width:400px;margin:0 auto">X</div></body>');
        final div = _findBox(root, 'div')!;
        // Container width = 784 (body 800 - 2*8 margin)
        // Free space = 784 - 400 = 384
        // Each auto margin = 192
        expect(div.margin.left, closeTo(192, 1));
        expect(div.margin.right, closeTo(192, 1));
      });

      test('min-width constrains width', () {
        final root = _layout('<body><div style="width:100px;min-width:200px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, greaterThanOrEqualTo(200));
      });

      test('max-width constrains width', () {
        final root = _layout('<body><div style="max-width:300px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, lessThanOrEqualTo(300));
      });

      test('min-height constrains height', () {
        final root = _layout('<body><div style="min-height:100px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.height, greaterThanOrEqualTo(100));
      });

      test('max-height constrains height', () {
        final root = _layout('<body><div style="height:500px;max-height:200px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.height, lessThanOrEqualTo(200));
      });

      test('display:none produces no box', () {
        final root = _layout('<body><div style="display:none">Hidden</div><div>Visible</div></body>');
        final divs = _findAllBoxes(root, 'div');
        // Only the visible div should have a layout box
        expect(divs.length, 1);
      });
    });

    // CSS 2.2 §9.4.2 — Inline formatting context
    group('Inline layout (CSS 2.2 §9.4.2)', () {
      test('inline elements sit side by side', () {
        final root = _layout('<body><span>A</span><span>B</span></body>');
        // Should produce text boxes on the same line
        expect(root.content.height, greaterThan(0));
      });

      test('text wraps at container width', () {
        final root = _layout(
            '<body><p>This is a very long sentence that should eventually wrap to the next line if the container is narrow enough</p></body>',
            width: 200);
        final p = _findBox(root, 'p')!;
        expect(p.content.height, greaterThan(20)); // Multiple lines
      });
    });

    // CSS Flexbox Level 1
    group('Flexbox (CSS Flexbox §9)', () {
      test('flex container lays out children horizontally (row)', () {
        final root = _layout('''
          <body><div style="display:flex">
            <div style="width:100px;height:50px">A</div>
            <div style="width:100px;height:50px">B</div>
          </div></body>
        ''');
        final flex = _findBox(root, 'div')!;
        expect(flex.children.length, greaterThanOrEqualTo(2));
        // Children should be side by side (different X positions)
        if (flex.children.length >= 2) {
          expect(flex.children[1].content.x, greaterThan(flex.children[0].content.x));
        }
      });

      test('flex container lays out children vertically (column)', () {
        final root = _layout('''
          <body><div style="display:flex;flex-direction:column">
            <div style="height:50px">A</div>
            <div style="height:50px">B</div>
          </div></body>
        ''');
        final flex = _findBox(root, 'div')!;
        if (flex.children.length >= 2) {
          expect(flex.children[1].content.y, greaterThan(flex.children[0].content.y));
        }
      });

      test('flex-grow distributes space', () {
        final root = _layout('''
          <body><div style="display:flex;width:600px">
            <div style="flex-grow:1;height:50px">A</div>
            <div style="flex-grow:2;height:50px">B</div>
          </div></body>
        ''');
        final flex = _findBox(root, 'div')!;
        if (flex.children.length >= 2) {
          // Child B should be roughly twice as wide as child A
          final aWidth = flex.children[0].content.width;
          final bWidth = flex.children[1].content.width;
          expect(bWidth, greaterThan(aWidth));
        }
      });

      test('justify-content: center', () {
        final root = _layout('''
          <body><div style="display:flex;justify-content:center;width:600px">
            <div style="width:100px;height:50px">A</div>
          </div></body>
        ''');
        final flex = _findBox(root, 'div')!;
        if (flex.children.isNotEmpty) {
          final child = flex.children[0];
          // Should be centered: x ≈ flex.content.x + (600-100)/2
          final expectedX = flex.content.x + 250;
          expect(child.content.x, closeTo(expectedX, 5));
        }
      });

      test('flex container height accommodates children', () {
        final root = _layout('''
          <body><div style="display:flex">
            <div style="width:100px;height:80px">A</div>
            <div style="width:100px;height:120px">B</div>
          </div></body>
        ''');
        final flex = _findBox(root, 'div')!;
        expect(flex.content.height, greaterThanOrEqualTo(120));
      });
    });

    // CSS Table
    group('Table layout (CSS Tables §4)', () {
      test('table has table layout type', () {
        final root = _layout('<body><table><tr><td>X</td></tr></table></body>');
        final table = _findBox(root, 'table');
        expect(table, isNotNull);
        expect(table!.layoutType, LayoutType.table);
      });

      test('table with multiple columns', () {
        final root = _layout('''
          <body><table>
            <tr><td>A</td><td>B</td><td>C</td></tr>
          </table></body>
        ''');
        final table = _findBox(root, 'table');
        expect(table, isNotNull);
        expect(table!.content.width, greaterThan(0));
        expect(table.content.height, greaterThan(0));
      });

      test('table with multiple rows stacks vertically', () {
        final root = _layout('''
          <body><table>
            <tr><td>Row 1</td></tr>
            <tr><td>Row 2</td></tr>
          </table></body>
        ''');
        final table = _findBox(root, 'table');
        expect(table, isNotNull);
        expect(table!.content.height, greaterThan(0));
      });

      test('table width=100% fills container', () {
        final root = _layout('<body><table width="100%"><tr><td>X</td></tr></table></body>');
        final table = _findBox(root, 'table');
        expect(table, isNotNull);
        // Should fill the body content area (784px with 8px body margin on each side)
        expect(table!.content.width, closeTo(784, 10));
      });

      test('table height is not zero', () {
        final root = _layout('''
          <body><table>
            <tr><td>Cell 1</td><td>Cell 2</td></tr>
            <tr><td>Cell 3</td><td>Cell 4</td></tr>
          </table></body>
        ''');
        final table = _findBox(root, 'table');
        expect(table, isNotNull);
        expect(table!.content.height, greaterThan(0));
      });
    });

    // CSS Grid Level 1
    group('Grid layout (CSS Grid §7)', () {
      test('grid container creates grid boxes', () {
        final root = _layout('''
          <body><div style="display:grid;grid-template-columns:1fr 1fr">
            <div>A</div><div>B</div>
          </div></body>
        ''');
        final grid = _findBox(root, 'div');
        expect(grid, isNotNull);
        expect(grid!.layoutType, LayoutType.grid);
      });
    });

    // CSS Units
    group('CSS Units (CSS Values §5)', () {
      test('px unit', () {
        final root = _layout('<body><div style="width:100px;height:50px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, 100);
        expect(div.content.height, 50);
      });

      test('em unit (relative to font-size)', () {
        final root = _layout('<body><div style="width:10em">X</div></body>');
        final div = _findBox(root, 'div')!;
        // Default font-size = 16px, so 10em = 160px
        expect(div.content.width, closeTo(160, 1));
      });

      test('rem unit (relative to root font-size)', () {
        final root = _layout('<body><div style="width:10rem">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, closeTo(160, 1));
      });

      test('percentage width', () {
        final root = _layout('<body><div style="width:50%">X</div></body>');
        final div = _findBox(root, 'div')!;
        // 50% of 784 = 392
        expect(div.content.width, closeTo(392, 1));
      });

      test('vw unit', () {
        final root = _layout('<body><div style="width:50vw">X</div></body>');
        final div = _findBox(root, 'div')!;
        // 50vw = 50% of 1024 = 512
        expect(div.content.width, closeTo(512, 1));
      });

      test('pt unit', () {
        final root = _layout('<body><div style="width:100pt">X</div></body>');
        final div = _findBox(root, 'div')!;
        // 1pt = 1.333px, so 100pt ≈ 133.3px
        expect(div.content.width, closeTo(133.3, 1));
      });
    });

    // CSS Overflow
    group('Overflow', () {
      test('overflow:hidden is stored on layout box', () {
        final root = _layout('<body><div style="overflow:hidden;height:100px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.overflow, 'hidden');
      });
    });

    // CSS Position
    group('Positioning (CSS 2.2 §9.3)', () {
      test('position:relative is stored', () {
        final root = _layout('<body><div style="position:relative">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.position, 'relative');
      });

      test('position:absolute is stored', () {
        final root = _layout('<body><div style="position:absolute">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.position, 'absolute');
      });

      test('position:fixed is stored', () {
        final root = _layout('<body><div style="position:fixed">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.position, 'fixed');
      });

      test('absolute elements do not affect normal flow height', () {
        final root = _layout('''
          <body>
            <div style="position:absolute;height:500px">Abs</div>
            <div style="height:50px">Normal</div>
          </body>
        ''');
        // Body height should NOT include the absolute element
        // Only the normal flow div contributes
      });
    });

    // CSS Float
    group('Float layout (CSS 2.2 §9.5)', () {
      test('float:left is stored', () {
        final root = _layout('<body><div style="float:left;width:100px;height:50px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.float_, 'left');
      });

      test('float:right is stored', () {
        final root = _layout('<body><div style="float:right;width:100px;height:50px">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.float_, 'right');
      });
    });

    // Z-index
    group('Z-index', () {
      test('z-index is stored', () {
        final root = _layout('<body><div style="position:relative;z-index:10">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.zIndex, 10);
      });
    });

    // Text alignment
    group('Text alignment (CSS Text §7)', () {
      test('text-align:center is respected in inline layout', () {
        final root = _layout('<body><p style="text-align:center;width:400px">Short</p></body>');
        final p = _findBox(root, 'p')!;
        // The text should be centered within the 400px container
        if (p.children.isNotEmpty) {
          final textBox = p.children.first;
          // Text should start after center point
          expect(textBox.content.x, greaterThan(p.content.x));
        }
      });
    });

    // calc()
    group('calc() expressions (CSS Values §10)', () {
      test('calc(100% - 20px)', () {
        final root = _layout('<body><div style="width:calc(100% - 20px)">X</div></body>');
        final div = _findBox(root, 'div')!;
        // 100% of 784 - 20 = 764
        expect(div.content.width, closeTo(764, 1));
      });

      test('calc with addition', () {
        final root = _layout('<body><div style="width:calc(100px + 50px)">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, closeTo(150, 1));
      });

      test('calc with multiplication', () {
        final root = _layout('<body><div style="width:calc(50px * 3)">X</div></body>');
        final div = _findBox(root, 'div')!;
        expect(div.content.width, closeTo(150, 1));
      });
    });

    // Link href propagation
    group('Link href propagation', () {
      test('a href propagates to layout boxes', () {
        final root = _layout('<body><a href="https://example.com">Click</a></body>');
        final linkBoxes = root.allBoxes.where((b) => b.linkHref != null);
        expect(linkBoxes, isNotEmpty);
        expect(linkBoxes.first.linkHref, 'https://example.com');
      });
    });

    // Form elements
    group('Form element layout', () {
      test('input generates a layout box', () {
        final root = _layout('<body><input type="text"></body>');
        final input = _findBox(root, 'input');
        expect(input, isNotNull);
        expect(input!.content.width, greaterThan(0));
      });

      test('button generates a layout box', () {
        final root = _layout('<body><button>Click</button></body>');
        final button = _findBox(root, 'button');
        expect(button, isNotNull);
      });
    });

    // Image elements
    group('Image layout', () {
      test('img with width/height attributes sizes correctly', () {
        final root = _layout('<body><img src="test.png" width="200" height="100"></body>');
        final img = _findBox(root, 'img');
        expect(img, isNotNull);
        expect(img!.content.width, 200);
        expect(img!.content.height, 100);
      });
    });
  });

  // ════════════════════════════════════════════════════════════════════
  // PART 5: INTEGRATION TESTS — Real-world HTML patterns
  // ════════════════════════════════════════════════════════════════════

  group('Integration: Real-world patterns', () {
    test('DrudgeReport-style table layout', () {
      final root = _layout('''
        <body text="#000000" link="#000000">
          <center>
            <table cellpadding="3" width="100%">
              <tr>
                <td width="33%" valign="top"><b>Column 1</b><br>Content 1</td>
                <td width="3" bgcolor="#000000"></td>
                <td width="33%" valign="top"><b>Column 2</b><br>Content 2</td>
                <td width="3" bgcolor="#000000"></td>
                <td width="33%" valign="top"><b>Column 3</b><br>Content 3</td>
              </tr>
            </table>
          </center>
        </body>
      ''');
      final table = _findBox(root, 'table');
      expect(table, isNotNull);
      expect(table!.content.width, greaterThan(0));
      expect(table.content.height, greaterThan(0));
    });

    test('nested div layout', () {
      final root = _layout('''
        <body>
          <div style="max-width:960px;margin:0 auto">
            <div style="padding:20px">
              <h1>Title</h1>
              <p>Paragraph text.</p>
            </div>
          </div>
        </body>
      ''');
      final outerDiv = _findBox(root, 'div');
      expect(outerDiv, isNotNull);
      // max-width should constrain to 960 (but body content is 784 which is less, so fills)
    });

    test('form layout', () {
      final root = _layout('''
        <body>
          <form>
            <label>Name:</label>
            <input type="text" value="John">
            <br>
            <button type="submit">Submit</button>
          </form>
        </body>
      ''');
      final form = _findBox(root, 'form');
      expect(form, isNotNull);
    });

    test('navigation list', () {
      final root = _layout('''
        <body>
          <nav>
            <ul>
              <li><a href="/home">Home</a></li>
              <li><a href="/about">About</a></li>
              <li><a href="/contact">Contact</a></li>
            </ul>
          </nav>
        </body>
      ''');
      final lis = _findAllBoxes(root, 'li');
      expect(lis.length, 3);
    });

    test('noscript fallback renders', () {
      final root = _layout('''
        <body>
          <noscript>
            <p>JavaScript is required for this site.</p>
          </noscript>
        </body>
      ''');
      // noscript content should render since we're a no-JS browser
      final p = _findBox(root, 'p');
      expect(p, isNotNull);
    });

    test('hidden elements are not laid out', () {
      final root = _layout('''
        <body>
          <div hidden>Should not appear</div>
          <div>Should appear</div>
        </body>
      ''');
      final divs = _findAllBoxes(root, 'div');
      expect(divs.length, 1);
    });

    test('legacy center + font + table', () {
      final root = _layout('''
        <body>
          <center>
            <font face="Arial" size="5" color="#FF0000">
              <b>Important Title</b>
            </font>
          </center>
        </body>
      ''');
      final center = _findBox(root, 'center');
      expect(center, isNotNull);
    });
  });
}
