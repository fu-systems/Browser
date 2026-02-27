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
  final measurer = StubMeasurer();

  // ── HTML Entity Tests ──────────────────────────────────────────────

  group('HTML entity decoding', () {
    test('decodes common named entities', () {
      final doc = HtmlParser.parse(
        '<p>&mdash; &ndash; &hellip; &copy; &reg; &trade;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u2014')); // mdash
      expect(text, contains('\u2013')); // ndash
      expect(text, contains('\u2026')); // hellip
      expect(text, contains('\u00A9')); // copy
      expect(text, contains('\u00AE')); // reg
      expect(text, contains('\u2122')); // trade
    });

    test('decodes quotation mark entities', () {
      final doc = HtmlParser.parse(
        '<p>&lsquo;hello&rsquo; &ldquo;world&rdquo;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u2018')); // lsquo
      expect(text, contains('\u2019')); // rsquo
      expect(text, contains('\u201C')); // ldquo
      expect(text, contains('\u201D')); // rdquo
    });

    test('decodes currency entities', () {
      final doc = HtmlParser.parse(
        '<p>&euro; &pound; &yen; &cent;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u20AC')); // euro
      expect(text, contains('\u00A3')); // pound
      expect(text, contains('\u00A5')); // yen
      expect(text, contains('\u00A2')); // cent
    });

    test('decodes arrow entities', () {
      final doc = HtmlParser.parse(
        '<p>&larr; &rarr; &uarr; &darr;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u2190')); // larr
      expect(text, contains('\u2192')); // rarr
      expect(text, contains('\u2191')); // uarr
      expect(text, contains('\u2193')); // darr
    });

    test('decodes fraction entities', () {
      final doc = HtmlParser.parse(
        '<p>&frac14; &frac12; &frac34;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u00BC')); // frac14
      expect(text, contains('\u00BD')); // frac12
      expect(text, contains('\u00BE')); // frac34
    });

    test('decodes Greek letter entities', () {
      final doc = HtmlParser.parse(
        '<p>&alpha; &beta; &pi; &omega; &Sigma; &Delta;</p>'
      );
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u03B1')); // alpha
      expect(text, contains('\u03B2')); // beta
      expect(text, contains('\u03C0')); // pi
      expect(text, contains('\u03C9')); // omega
      expect(text, contains('\u03A3')); // Sigma
      expect(text, contains('\u0394')); // Delta
    });

    test('numeric entities still work', () {
      final doc = HtmlParser.parse('<p>&#169; &#x2665;</p>');
      final text = (doc.body!.children.first as Element).textContent;
      expect(text, contains('\u00A9')); // &#169; = copyright
      expect(text, contains('\u2665')); // &#x2665; = heart
    });
  });

  // ── Auto-closing Tests ─────────────────────────────────────────────

  group('Parser auto-closing', () {
    test('p auto-closes when another block opens', () {
      final doc = HtmlParser.parse(
        '<body><p>First<div>Block</div><p>Second</p></body>'
      );
      final body = doc.body!;
      // body should have: <p>First</p>, <div>Block</div>, <p>Second</p>
      final elements = body.children.whereType<Element>().toList();
      expect(elements.length, 3);
      expect(elements[0].tagName, 'p');
      expect(elements[0].textContent.trim(), 'First');
      expect(elements[1].tagName, 'div');
      expect(elements[2].tagName, 'p');
    });

    test('li auto-closes when another li opens', () {
      final doc = HtmlParser.parse(
        '<ul><li>One<li>Two<li>Three</ul>'
      );
      final ul = doc.body!.children.whereType<Element>().first;
      final lis = ul.children.whereType<Element>().where((e) => e.tagName == 'li').toList();
      expect(lis.length, 3);
      expect(lis[0].textContent.trim(), 'One');
      expect(lis[1].textContent.trim(), 'Two');
      expect(lis[2].textContent.trim(), 'Three');
    });

    test('td auto-closes when another td opens', () {
      final doc = HtmlParser.parse(
        '<table><tr><td>A<td>B<td>C</tr></table>'
      );
      final table = doc.body!.children.whereType<Element>().first;
      final tr = table.children.whereType<Element>().first;
      final tds = tr.children.whereType<Element>().where((e) => e.tagName == 'td').toList();
      expect(tds.length, 3);
      expect(tds[0].textContent.trim(), 'A');
      expect(tds[1].textContent.trim(), 'B');
      expect(tds[2].textContent.trim(), 'C');
    });

    test('dd/dt auto-close each other', () {
      final doc = HtmlParser.parse(
        '<dl><dt>Term1<dd>Def1<dt>Term2<dd>Def2</dl>'
      );
      final dl = doc.body!.children.whereType<Element>().first;
      final children = dl.children.whereType<Element>().toList();
      expect(children.length, 4);
      expect(children[0].tagName, 'dt');
      expect(children[1].tagName, 'dd');
      expect(children[2].tagName, 'dt');
      expect(children[3].tagName, 'dd');
    });

    test('option auto-closes when another option opens', () {
      final doc = HtmlParser.parse(
        '<select><option>A<option>B<option>C</select>'
      );
      final select = doc.body!.children.whereType<Element>().first;
      final options = select.children.whereType<Element>().where((e) => e.tagName == 'option').toList();
      expect(options.length, 3);
    });
  });

  // ── Block Element Classification Tests ─────────────────────────────

  group('Block element classification', () {
    test('new block elements are in blockElements set', () {
      expect(blockElements, contains('address'));
      expect(blockElements, contains('dialog'));
      expect(blockElements, contains('menu'));
      expect(blockElements, contains('hgroup'));
      expect(blockElements, contains('search'));
      expect(blockElements, contains('colgroup'));
    });

    test('address renders as block', () {
      final doc = HtmlParser.parse(
        '<body><address>Contact info</address></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final address = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'address'
      );
      expect(address.display, Display.block);
      expect(address.prop('font-style'), 'italic');
    });
  });

  // ── Inline Text Semantics Style Tests ──────────────────────────────

  group('Inline text semantics defaults', () {
    test('small gets smaller font-size', () {
      final doc = HtmlParser.parse('<body><small>Small text</small></body>');
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final small = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'small'
      );
      expect(small.prop('font-size'), '13px');
    });

    test('s/del/strike get line-through', () {
      for (final tag in ['s', 'del', 'strike']) {
        final doc = HtmlParser.parse('<body><$tag>deleted</$tag></body>');
        final styled = computeStyles(doc.body!, <Stylesheet>[]);
        final el = styled.children.firstWhere(
          (c) => c.node is Element && (c.node as Element).tagName == tag
        );
        expect(el.prop('text-decoration'), 'line-through',
            reason: '<$tag> should have line-through');
      }
    });

    test('sub/sup get smaller font and vertical-align', () {
      final doc = HtmlParser.parse(
        '<body><sub>sub</sub><sup>sup</sup></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final sub = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'sub'
      );
      final sup = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'sup'
      );
      expect(sub.prop('vertical-align'), 'sub');
      expect(sub.prop('font-size'), '13px');
      expect(sup.prop('vertical-align'), 'super');
      expect(sup.prop('font-size'), '13px');
    });

    test('mark gets yellow background', () {
      final doc = HtmlParser.parse('<body><mark>highlighted</mark></body>');
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final mark = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'mark'
      );
      expect(mark.prop('background-color'), '#ffff00');
    });

    test('kbd/samp get monospace', () {
      for (final tag in ['kbd', 'samp']) {
        final doc = HtmlParser.parse('<body><$tag>code</$tag></body>');
        final styled = computeStyles(doc.body!, <Stylesheet>[]);
        final el = styled.children.firstWhere(
          (c) => c.node is Element && (c.node as Element).tagName == tag
        );
        expect(el.prop('font-family'), 'monospace',
            reason: '<$tag> should have monospace');
      }
    });

    test('cite/var/dfn get italic', () {
      for (final tag in ['cite', 'var', 'dfn']) {
        final doc = HtmlParser.parse('<body><$tag>text</$tag></body>');
        final styled = computeStyles(doc.body!, <Stylesheet>[]);
        final el = styled.children.firstWhere(
          (c) => c.node is Element && (c.node as Element).tagName == tag
        );
        expect(el.prop('font-style'), 'italic',
            reason: '<$tag> should be italic');
      }
    });

    test('ins gets underline', () {
      final doc = HtmlParser.parse('<body><ins>inserted</ins></body>');
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final ins = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'ins'
      );
      expect(ins.prop('text-decoration'), 'underline');
    });

    test('abbr gets dotted underline', () {
      final doc = HtmlParser.parse('<body><abbr>HTML</abbr></body>');
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final abbr = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'abbr'
      );
      expect(abbr.prop('text-decoration'), 'underline dotted');
    });

    test('big gets larger font-size', () {
      final doc = HtmlParser.parse('<body><big>Big text</big></body>');
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final big = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'big'
      );
      expect(big.prop('font-size'), '19px');
    });
  });

  // ── Structural Defaults Tests ──────────────────────────────────────

  group('Structural element defaults', () {
    test('dd gets left margin', () {
      final doc = HtmlParser.parse(
        '<body><dl><dt>Term</dt><dd>Definition</dd></dl></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      StyledNode? findTag(StyledNode node, String tag) {
        if (node.node is Element && (node.node as Element).tagName == tag) {
          return node;
        }
        for (final child in node.children) {
          final found = findTag(child, tag);
          if (found != null) return found;
        }
        return null;
      }
      final dd = findTag(styled, 'dd');
      expect(dd, isNotNull);
      expect(dd!.prop('margin-left'), '40px');
    });

    test('figure gets margins', () {
      final doc = HtmlParser.parse(
        '<body><figure><img src="x"><figcaption>Caption</figcaption></figure></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final figure = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'figure'
      );
      expect(figure.prop('margin-left'), '40px');
      expect(figure.prop('margin-top'), '16px');
    });

    test('fieldset gets border and padding', () {
      final doc = HtmlParser.parse(
        '<body><fieldset><legend>Group</legend></fieldset></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final fieldset = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'fieldset'
      );
      expect(fieldset.prop('border'), isNotEmpty);
    });

    test('caption gets text-align center', () {
      final doc = HtmlParser.parse(
        '<body><table><caption>Title</caption></table></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      StyledNode? findTag(StyledNode node, String tag) {
        if (node.node is Element && (node.node as Element).tagName == tag) {
          return node;
        }
        for (final child in node.children) {
          final found = findTag(child, tag);
          if (found != null) return found;
        }
        return null;
      }
      final caption = findTag(styled, 'caption');
      expect(caption, isNotNull);
      expect(caption!.prop('text-align'), 'center');
    });

    test('th gets bold and center', () {
      final doc = HtmlParser.parse(
        '<body><table><tr><th>Header</th></tr></table></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      StyledNode? findTag(StyledNode node, String tag) {
        if (node.node is Element && (node.node as Element).tagName == tag) {
          return node;
        }
        for (final child in node.children) {
          final found = findTag(child, tag);
          if (found != null) return found;
        }
        return null;
      }
      final th = findTag(styled, 'th');
      expect(th, isNotNull);
      expect(th!.prop('font-weight'), 'bold');
      expect(th.prop('text-align'), 'center');
    });
  });

  // ── Media / Embedded Element Layout Tests ──────────────────────────

  group('Media element layout', () {
    test('video produces replaced box with defaults', () {
      final doc = HtmlParser.parse(
        '<body><video src="movie.mp4"></video></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final video = allBoxes.firstWhere((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'video'
      );
      expect(video.imageUrl, 'movie.mp4');
      expect(video.imageWidth, 300);
      expect(video.imageHeight, 150);
    });

    test('audio produces replaced box with correct height', () {
      final doc = HtmlParser.parse(
        '<body><audio src="song.mp3"></audio></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final audio = allBoxes.firstWhere((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'audio'
      );
      expect(audio.imageWidth, 300);
      expect(audio.imageHeight, 32);
    });

    test('iframe produces replaced box with custom dimensions', () {
      final doc = HtmlParser.parse(
        '<body><iframe src="page.html" width="640" height="480"></iframe></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final iframe = allBoxes.firstWhere((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'iframe'
      );
      expect(iframe.imageWidth, 640);
      expect(iframe.imageHeight, 480);
    });

    test('canvas produces replaced box', () {
      final doc = HtmlParser.parse(
        '<body><canvas width="200" height="100"></canvas></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final canvas = allBoxes.firstWhere((b) =>
        b.styledNode?.node is Element &&
        (b.styledNode!.node as Element).tagName == 'canvas'
      );
      expect(canvas.imageWidth, 200);
      expect(canvas.imageHeight, 100);
    });
  });

  // ── Form Element Layout Tests ──────────────────────────────────────

  group('Extended form element layout', () {
    test('progress produces form box', () {
      final doc = HtmlParser.parse(
        '<body><progress value="70" max="100"></progress></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final progress = allBoxes.firstWhere((b) => b.formTag == 'progress');
      expect(progress.content.width, greaterThan(0));
      expect(progress.content.height, greaterThan(0));
    });

    test('meter produces form box', () {
      final doc = HtmlParser.parse(
        '<body><meter value="0.6"></meter></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final meter = allBoxes.firstWhere((b) => b.formTag == 'meter');
      expect(meter.content.width, greaterThan(0));
    });

    test('range input sizing', () {
      final doc = HtmlParser.parse(
        '<body><input type="range"></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final root = layoutTree(styled, 800, measurer);
      final allBoxes = root.allBoxes.toList();
      final range = allBoxes.firstWhere((b) =>
        b.formTag == 'input' && b.formType == 'range'
      );
      expect(range.content.width, 160);
    });
  });

  // ── Font color attribute test ──────────────────────────────────────

  group('Legacy HTML attributes', () {
    test('font color attribute maps to CSS color', () {
      final doc = HtmlParser.parse(
        '<body><font color="red">Red text</font></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final font = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'font'
      );
      expect(font.prop('color'), 'red');
    });

    test('font face attribute maps to font-family', () {
      final doc = HtmlParser.parse(
        '<body><font face="Arial,Helvetica">Text</font></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final font = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'font'
      );
      expect(font.prop('font-family'), 'Arial,Helvetica');
    });

    test('font size attribute maps to font-size', () {
      final doc = HtmlParser.parse(
        '<body><font size="+2">Big</font></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final font = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'font'
      );
      // size +2 → 3+2=5 → 24px
      expect(font.prop('font-size'), '24px');
    });

    test('HTML width/height attributes on table', () {
      final doc = HtmlParser.parse(
        '<body><table width="500" height="200"></table></body>'
      );
      final styled = computeStyles(doc.body!, <Stylesheet>[]);
      final table = styled.children.firstWhere(
        (c) => c.node is Element && (c.node as Element).tagName == 'table'
      );
      expect(table.prop('width'), '500px');
      expect(table.prop('height'), '200px');
    });
  });

  // ── Comprehensive element rendering test ───────────────────────────

  test('Comprehensive HTML: all element types produce valid layout', () {
    final html = '''
    <body>
      <h1>Heading 1</h1>
      <h2>Heading 2</h2>
      <h3>Heading 3</h3>
      <p>A paragraph with <strong>bold</strong>, <em>italic</em>,
         <small>small</small>, <sub>sub</sub>, <sup>sup</sup>,
         <s>strikethrough</s>, <mark>highlighted</mark>,
         <code>code</code>, <kbd>keyboard</kbd>,
         <abbr>abbr</abbr>, <cite>cite</cite>,
         <var>variable</var>, <dfn>definition</dfn>,
         <ins>inserted</ins>, <del>deleted</del>,
         <u>underline</u>, <big>big</big> text.</p>
      <blockquote>A blockquote</blockquote>
      <pre>Preformatted text</pre>
      <address>Contact info here</address>
      <hr>
      <ul><li>Item 1<li>Item 2<li>Item 3</ul>
      <ol><li>First<li>Second<li>Third</ol>
      <dl><dt>Term<dd>Definition</dl>
      <figure>
        <img src="photo.jpg" width="200" height="100">
        <figcaption>A caption</figcaption>
      </figure>
      <table>
        <caption>Table title</caption>
        <tr><th>Name<th>Value</tr>
        <tr><td>A<td>1</tr>
        <tr><td>B<td>2</tr>
      </table>
      <form>
        <fieldset>
          <legend>Form</legend>
          <input type="text" placeholder="Name">
          <input type="checkbox">
          <input type="radio">
          <input type="range">
          <input type="color">
          <select><option>A<option>B</select>
          <textarea>Content</textarea>
          <button>Submit</button>
        </fieldset>
      </form>
      <details><summary>Expand</summary>Details here</details>
      <video src="movie.mp4" width="320" height="240"></video>
      <audio src="song.mp3"></audio>
      <iframe src="frame.html" width="400" height="300"></iframe>
      <canvas width="150" height="75"></canvas>
      <progress value="50" max="100"></progress>
      <meter value="0.7"></meter>
    </body>
    ''';

    final doc = HtmlParser.parse(html);
    final styledTree = computeStyles(doc.body ?? doc, <Stylesheet>[]);
    final root = layoutTree(styledTree, 1024, measurer);

    final allBoxes = root.allBoxes.toList();
    final totalBoxes = allBoxes.length;

    print('=== Comprehensive Element Test ===');
    print('Total boxes: $totalBoxes');

    // Count by type
    int blocks = 0, inlines = 0, texts = 0, anons = 0;
    for (final box in allBoxes) {
      if (box.layoutType == LayoutType.block) blocks++;
      if (box.layoutType == LayoutType.inline) inlines++;
      if (box.layoutType == LayoutType.text) texts++;
      if (box.layoutType == LayoutType.anonymous) anons++;
    }
    print('  block=$blocks inline=$inlines text=$texts anon=$anons');

    // Should produce many boxes (all elements present).
    expect(totalBoxes, greaterThan(60),
        reason: 'Should produce many layout boxes from comprehensive HTML');

    // Verify no negative positions
    for (final box in allBoxes) {
      expect(box.content.x, greaterThanOrEqualTo(0),
          reason: 'No box should have negative x');
      expect(box.content.y, greaterThanOrEqualTo(0),
          reason: 'No box should have negative y');
    }

    // Check that major block elements stack vertically (increasing y).
    final topLevelBlocks = root.children.where((c) =>
        c.layoutType == LayoutType.block || c.layoutType == LayoutType.anonymous
    ).toList();
    for (int i = 1; i < topLevelBlocks.length; i++) {
      final prev = topLevelBlocks[i - 1];
      final curr = topLevelBlocks[i];
      if (prev.content.height > 0 && curr.content.height > 0) {
        expect(curr.content.y, greaterThanOrEqualTo(prev.content.y),
            reason: 'Block ${i} should be at or below block ${i - 1}');
      }
    }

    // Check media elements are present
    final mediaTypes = <String>{};
    for (final box in allBoxes) {
      if (box.styledNode?.node is Element) {
        final tag = (box.styledNode!.node as Element).tagName;
        if ({'video', 'audio', 'iframe', 'canvas'}.contains(tag)) {
          mediaTypes.add(tag);
        }
      }
    }
    expect(mediaTypes, containsAll(['video', 'audio', 'iframe', 'canvas']),
        reason: 'All media elements should produce layout boxes');

    // Check form elements are present
    final formTags = <String>{};
    for (final box in allBoxes) {
      if (box.formTag != null) formTags.add(box.formTag!);
    }
    expect(formTags, containsAll(['input', 'select', 'textarea', 'button', 'progress', 'meter']),
        reason: 'All form elements should produce layout boxes');

    print('All comprehensive element checks passed!');
  });
}
