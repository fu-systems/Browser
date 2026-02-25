import 'package:flutter_test/flutter_test.dart';
import 'package:pane/engine/dom.dart';
import 'package:pane/engine/html_parser.dart';

void main() {
  group('HtmlParser', () {
    test('parses a minimal HTML document', () {
      final doc = HtmlParser.parse('<html><head></head><body><p>Hello</p></body></html>');
      expect(doc.documentElement, isNotNull);
      expect(doc.documentElement!.tagName, 'html');
      expect(doc.body, isNotNull);
      expect(doc.head, isNotNull);
    });

    test('auto-creates html/head/body when missing', () {
      final doc = HtmlParser.parse('<p>Hello World</p>');
      expect(doc.documentElement!.tagName, 'html');
      expect(doc.body, isNotNull);
      // The <p> should be inside <body>.
      final body = doc.body!;
      final p = body.children.whereType<Element>().firstOrNull;
      expect(p, isNotNull);
      expect(p!.tagName, 'p');
    });

    test('parses attributes', () {
      final doc = HtmlParser.parse('<a href="https://example.com" class="link">Click</a>');
      final body = doc.body!;
      final a = body.elementDescendants.where((e) => e.tagName == 'a').first;
      expect(a.attributes['href'], 'https://example.com');
      expect(a.attributes['class'], 'link');
    });

    test('handles self-closing tags', () {
      final doc = HtmlParser.parse('<p>Before<br/>After</p>');
      final body = doc.body!;
      final p = body.elementDescendants.where((e) => e.tagName == 'p').first;
      expect(p.children.length, greaterThanOrEqualTo(2));
      expect(p.elementDescendants.any((e) => e.tagName == 'br'), isTrue);
    });

    test('handles void elements without slash', () {
      final doc = HtmlParser.parse('<img src="test.png"><br><hr>');
      final body = doc.body!;
      final tags = body.elementDescendants.map((e) => e.tagName).toSet();
      expect(tags, containsAll(['img', 'br', 'hr']));
    });

    test('parses nested elements', () {
      final doc = HtmlParser.parse('''
        <div>
          <h1>Title</h1>
          <p>Paragraph with <strong>bold</strong> text</p>
        </div>
      ''');
      final body = doc.body!;
      final div = body.elementDescendants.where((e) => e.tagName == 'div').first;
      final h1 = div.elementDescendants.where((e) => e.tagName == 'h1').first;
      expect(h1.textContent, 'Title');
      final strong = div.elementDescendants.where((e) => e.tagName == 'strong').first;
      expect(strong.textContent, 'bold');
    });

    test('extracts page title', () {
      final doc = HtmlParser.parse('<html><head><title>My Page</title></head><body></body></html>');
      expect(doc.title, 'My Page');
    });

    test('collects style element content', () {
      final doc = HtmlParser.parse('''
        <html><head><style>p { color: red; }</style></head><body></body></html>
      ''');
      expect(doc.internalCSS, contains('color: red'));
    });

    test('collects external stylesheet URLs', () {
      final doc = HtmlParser.parse('''
        <html><head><link rel="stylesheet" href="style.css"></head><body></body></html>
      ''');
      expect(doc.externalStylesheetUrls, contains('style.css'));
    });

    test('ignores script elements', () {
      final doc = HtmlParser.parse('<body><script>alert("hi")</script><p>Hello</p></body>');
      final body = doc.body!;
      expect(body.elementDescendants.any((e) => e.tagName == 'script'), isFalse);
    });

    test('handles doctype', () {
      final doc = HtmlParser.parse('<!DOCTYPE html><html><body><p>Hello</p></body></html>');
      expect(doc.body, isNotNull);
    });

    test('decodes HTML entities', () {
      final doc = HtmlParser.parse('<p>&amp; &lt; &gt; &quot;</p>');
      final body = doc.body!;
      final p = body.elementDescendants.where((e) => e.tagName == 'p').first;
      expect(p.textContent, contains('&'));
      expect(p.textContent, contains('<'));
      expect(p.textContent, contains('>'));
    });

    test('handles unquoted attributes', () {
      final doc = HtmlParser.parse('<div id=main class=container>Content</div>');
      final body = doc.body!;
      final div = body.elementDescendants.where((e) => e.tagName == 'div').first;
      expect(div.id, 'main');
      expect(div.classes, contains('container'));
    });

    test('handles boolean attributes', () {
      final doc = HtmlParser.parse('<input disabled>');
      final body = doc.body!;
      final input = body.elementDescendants.where((e) => e.tagName == 'input').first;
      expect(input.attributes.containsKey('disabled'), isTrue);
    });
  });

  group('DOM', () {
    test('Element.textContent concatenates all descendant text', () {
      final div = Element('div');
      div.appendChild(Text('Hello '));
      final span = Element('span');
      span.appendChild(Text('World'));
      div.appendChild(span);
      expect(div.textContent, 'Hello World');
    });

    test('Element.classes splits class attribute', () {
      final el = Element('div', {'class': 'foo bar baz'});
      expect(el.classes, ['foo', 'bar', 'baz']);
    });

    test('Node.descendants iterates depth-first', () {
      final root = Element('div');
      final child1 = Element('p');
      final child2 = Element('span');
      final grandchild = Element('a');
      child1.appendChild(grandchild);
      root.appendChild(child1);
      root.appendChild(child2);

      final tags = root.elementDescendants.map((e) => e.tagName).toList();
      expect(tags, ['p', 'a', 'span']);
    });
  });
}
