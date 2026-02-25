/// Page painter — renders the layout tree onto a Flutter Canvas.
///
/// Walks the LayoutBox tree and draws backgrounds, borders, text,
/// images, and form element placeholders using Flutter's Canvas API.

import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import '../engine/layout.dart' as engine;
import '../engine/style.dart';
import '../engine/dom.dart' as dom;

class PagePainter extends CustomPainter {
  final engine.LayoutBox? rootBox;
  final double scrollOffset;
  final Map<String, ui.Image> imageCache;
  final String searchQuery;
  final int currentSearchIndex;
  final List<engine.Rect> searchRects;

  PagePainter({
    this.rootBox,
    this.scrollOffset = 0,
    this.imageCache = const {},
    this.searchQuery = '',
    this.currentSearchIndex = -1,
    this.searchRects = const [],
  });

  @override
  void paint(Canvas canvas, Size size) {
    if (rootBox == null) return;

    canvas.save();
    canvas.translate(0, -scrollOffset);

    _paintBox(canvas, rootBox!);

    // Paint search highlights.
    if (searchRects.isNotEmpty) {
      for (int i = 0; i < searchRects.length; i++) {
        final r = searchRects[i];
        final paint = Paint()
          ..color = i == currentSearchIndex
              ? Colors.orange.withValues(alpha: 0.6)
              : Colors.yellow.withValues(alpha: 0.4);
        canvas.drawRect(
          ui.Rect.fromLTWH(r.x, r.y, r.width, r.height),
          paint,
        );
      }
    }

    canvas.restore();
  }

  void _paintBox(Canvas canvas, engine.LayoutBox box) {
    _paintBackground(canvas, box);
    _paintBorders(canvas, box);
    _paintHr(canvas, box);

    if (box.imageUrl != null && box.imageUrl!.isNotEmpty) {
      _paintImage(canvas, box);
    }

    if (box.formTag != null) {
      _paintFormElement(canvas, box);
    }

    if (box.text != null && box.text!.isNotEmpty) {
      _paintText(canvas, box);
    }

    for (final child in box.children) {
      _paintBox(canvas, child);
    }
  }

  void _paintBackground(Canvas canvas, engine.LayoutBox box) {
    final bgColor = _resolveColor(box.styledNode?['background-color'] ?? box.styledNode?['background'] ?? '');
    if (bgColor == null) return;

    final rect = box.paddingBox;
    canvas.drawRect(
      ui.Rect.fromLTWH(rect.x, rect.y, rect.width, rect.height),
      Paint()..color = bgColor,
    );
  }

  void _paintBorders(Canvas canvas, engine.LayoutBox box) {
    final bw = box.border;
    if (bw.top == 0 && bw.right == 0 && bw.bottom == 0 && bw.left == 0) return;

    final borderColor = _resolveBorderColor(box.styledNode);
    final rect = box.borderBox;

    final paint = Paint()
      ..color = borderColor
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;

    if (bw.top > 0) {
      paint.strokeWidth = bw.top;
      canvas.drawLine(Offset(rect.x, rect.y), Offset(rect.x + rect.width, rect.y), paint);
    }
    if (bw.right > 0) {
      paint.strokeWidth = bw.right;
      canvas.drawLine(Offset(rect.x + rect.width, rect.y), Offset(rect.x + rect.width, rect.y + rect.height), paint);
    }
    if (bw.bottom > 0) {
      paint.strokeWidth = bw.bottom;
      canvas.drawLine(Offset(rect.x, rect.y + rect.height), Offset(rect.x + rect.width, rect.y + rect.height), paint);
    }
    if (bw.left > 0) {
      paint.strokeWidth = bw.left;
      canvas.drawLine(Offset(rect.x, rect.y), Offset(rect.x, rect.y + rect.height), paint);
    }
  }

  void _paintHr(Canvas canvas, engine.LayoutBox box) {
    if (box.styledNode?.node is! dom.Element) return;
    if ((box.styledNode!.node as dom.Element).tagName != 'hr') return;

    final r = box.content;
    canvas.drawLine(
      Offset(r.x, r.y + r.height / 2),
      Offset(r.x + r.width, r.y + r.height / 2),
      Paint()..color = Colors.grey,
    );
  }

  void _paintImage(Canvas canvas, engine.LayoutBox box) {
    final r = box.content;
    final img = imageCache[box.imageUrl];
    if (img != null) {
      final src = ui.Rect.fromLTWH(0, 0, img.width.toDouble(), img.height.toDouble());
      final dst = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
      canvas.drawImageRect(img, src, dst, Paint());
    } else {
      final rect = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
      canvas.drawRect(rect, Paint()..color = const Color(0xFFF0F0F0));
      canvas.drawRect(
        rect,
        Paint()
          ..color = const Color(0xFFCCCCCC)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1,
      );
      final iconPaint = Paint()..color = const Color(0xFFAAAAAA);
      final cx = r.x + r.width / 2;
      final cy = r.y + r.height / 2;
      canvas.drawRect(ui.Rect.fromCenter(center: Offset(cx, cy), width: 16, height: 12), iconPaint);
    }
  }

  void _paintFormElement(Canvas canvas, engine.LayoutBox box) {
    final r = box.content;
    final tag = box.formTag!;
    final type = box.formType ?? 'text';

    if (type == 'hidden') return;

    if (type == 'checkbox') {
      final rect = ui.Rect.fromLTWH(r.x, r.y, 14, 14);
      canvas.drawRect(rect, Paint()..color = Colors.white);
      canvas.drawRect(rect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1.5);
      return;
    }

    if (type == 'radio') {
      canvas.drawCircle(Offset(r.x + 7, r.y + 7), 7, Paint()..color = Colors.white);
      canvas.drawCircle(Offset(r.x + 7, r.y + 7), 7, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1.5);
      return;
    }

    if (tag == 'button' || type == 'submit' || type == 'reset' || type == 'button') {
      final rrect = RRect.fromRectAndRadius(
        ui.Rect.fromLTWH(r.x, r.y, r.width, r.height),
        const Radius.circular(3),
      );
      canvas.drawRRect(rrect, Paint()..color = const Color(0xFFE8E8E8));
      canvas.drawRRect(rrect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1);
      final label = box.formValue?.isNotEmpty == true ? box.formValue! : (type == 'submit' ? 'Submit' : 'Button');
      _drawLabel(canvas, label, r, Colors.black, 12);
      return;
    }

    if (tag == 'select') {
      final rect = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
      canvas.drawRect(rect, Paint()..color = Colors.white);
      canvas.drawRect(rect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1);
      final arrowX = r.x + r.width - 16;
      final arrowY = r.y + r.height / 2;
      final path = Path()
        ..moveTo(arrowX, arrowY - 3)
        ..lineTo(arrowX + 8, arrowY - 3)
        ..lineTo(arrowX + 4, arrowY + 3)
        ..close();
      canvas.drawPath(path, Paint()..color = Colors.grey);
      return;
    }

    if (tag == 'textarea') {
      final rect = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
      canvas.drawRect(rect, Paint()..color = Colors.white);
      canvas.drawRect(rect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1);
      if (box.formPlaceholder?.isNotEmpty == true) {
        _drawLabel(canvas, box.formPlaceholder!, r, Colors.grey, 12);
      }
      return;
    }

    // Default: text input.
    final rect = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
    canvas.drawRect(rect, Paint()..color = Colors.white);
    canvas.drawRect(rect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1);
    if (box.formPlaceholder?.isNotEmpty == true) {
      _drawLabel(canvas, box.formPlaceholder!, r, Colors.grey.shade400, 12);
    }
  }

  void _drawLabel(Canvas canvas, String text, engine.Rect r, Color color, double fontSize) {
    final painter = TextPainter(
      text: TextSpan(
        text: text,
        style: TextStyle(color: color, fontSize: fontSize),
      ),
      textDirection: ui.TextDirection.ltr,
    );
    painter.layout(maxWidth: r.width - 8);
    painter.paint(canvas, Offset(r.x + 4, r.y + (r.height - painter.height) / 2));
    painter.dispose();
  }

  void _paintText(Canvas canvas, engine.LayoutBox box) {
    final styled = box.styledNode;
    final text = box.text!;

    final fontSize = _parsePx(styled?['font-size'] ?? '16px');
    final fontFamily = styled?['font-family'] ?? 'serif';
    final fontWeight = (styled?['font-weight'] ?? 'normal').toLowerCase();
    final fontStyle = (styled?['font-style'] ?? 'normal').toLowerCase();
    final color = _resolveColor(styled?['color'] ?? '#000000') ?? Colors.black;
    final decoration = styled?['text-decoration'] ?? '';

    final style = TextStyle(
      fontSize: fontSize,
      fontFamily: _mapFontFamily(fontFamily),
      fontWeight: fontWeight == 'bold' || fontWeight == '700'
          ? FontWeight.bold
          : FontWeight.normal,
      fontStyle: fontStyle == 'italic' ? FontStyle.italic : FontStyle.normal,
      color: color,
      decoration: decoration.contains('underline')
          ? TextDecoration.underline
          : TextDecoration.none,
      height: 1.4,
    );

    final span = TextSpan(text: text, style: style);
    final painter = TextPainter(
      text: span,
      textDirection: ui.TextDirection.ltr,
    );
    painter.layout(maxWidth: box.content.width > 0 ? box.content.width : double.infinity);
    painter.paint(canvas, Offset(box.content.x, box.content.y));
    painter.dispose();
  }

  Color _resolveBorderColor(StyledNode? styled) {
    if (styled == null) return Colors.black;
    final bc = styled['border-color'];
    if (bc != null) return _resolveColor(bc) ?? Colors.black;
    final border = styled['border'] ?? styled['border-top'] ?? '';
    if (border.isNotEmpty) {
      final parts = border.split(RegExp(r'\s+'));
      if (parts.length >= 3) return _resolveColor(parts[2]) ?? Colors.black;
    }
    return Colors.black;
  }

  @override
  bool shouldRepaint(covariant PagePainter oldDelegate) {
    return oldDelegate.rootBox != rootBox ||
        oldDelegate.scrollOffset != scrollOffset ||
        oldDelegate.searchQuery != searchQuery ||
        oldDelegate.currentSearchIndex != currentSearchIndex ||
        oldDelegate.imageCache.length != imageCache.length;
  }
}

// ── Color parsing ───────────────────────────────────────────────────

Color? _resolveColor(String value) {
  if (value.isEmpty || value == 'transparent') return null;

  final named = _namedColors[value.toLowerCase()];
  if (named != null) return named;

  if (value.startsWith('#')) {
    final hex = value.substring(1);
    if (hex.length == 3) {
      final r = int.parse(hex[0] * 2, radix: 16);
      final g = int.parse(hex[1] * 2, radix: 16);
      final b = int.parse(hex[2] * 2, radix: 16);
      return Color.fromARGB(255, r, g, b);
    }
    if (hex.length == 6) {
      final n = int.parse(hex, radix: 16);
      return Color.fromARGB(255, (n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
    }
    if (hex.length == 8) {
      final n = int.parse(hex, radix: 16);
      return Color.fromARGB(
        (n >> 24) & 0xFF,
        (n >> 16) & 0xFF,
        (n >> 8) & 0xFF,
        n & 0xFF,
      );
    }
  }

  final rgbMatch = RegExp(r'rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+))?\s*\)').firstMatch(value);
  if (rgbMatch != null) {
    final r = int.parse(rgbMatch.group(1)!);
    final g = int.parse(rgbMatch.group(2)!);
    final b = int.parse(rgbMatch.group(3)!);
    final a = rgbMatch.group(4) != null
        ? (double.parse(rgbMatch.group(4)!) * 255).round()
        : 255;
    return Color.fromARGB(a, r, g, b);
  }

  return null;
}

double _parsePx(String value) {
  if (value.endsWith('px')) {
    return double.tryParse(value.replaceAll('px', '')) ?? 16;
  }
  if (value.endsWith('em')) {
    final n = double.tryParse(value.replaceAll('em', ''));
    if (n != null) return n * 16;
  }
  return double.tryParse(value) ?? 16;
}

String _mapFontFamily(String family) {
  switch (family.toLowerCase().split(',').first.trim()) {
    case 'serif':
      return 'Serif';
    case 'sans-serif':
      return 'Sans';
    case 'monospace':
      return 'Monospace';
    default:
      return family;
  }
}

const _namedColors = <String, Color>{
  'black': Color(0xFF000000),
  'white': Color(0xFFFFFFFF),
  'red': Color(0xFFFF0000),
  'green': Color(0xFF008000),
  'blue': Color(0xFF0000FF),
  'yellow': Color(0xFFFFFF00),
  'cyan': Color(0xFF00FFFF),
  'magenta': Color(0xFFFF00FF),
  'orange': Color(0xFFFFA500),
  'purple': Color(0xFF800080),
  'gray': Color(0xFF808080),
  'grey': Color(0xFF808080),
  'silver': Color(0xFFC0C0C0),
  'navy': Color(0xFF000080),
  'teal': Color(0xFF008080),
  'maroon': Color(0xFF800000),
  'olive': Color(0xFF808000),
  'lime': Color(0xFF00FF00),
  'aqua': Color(0xFF00FFFF),
  'fuchsia': Color(0xFFFF00FF),
  'lightgray': Color(0xFFD3D3D3),
  'lightgrey': Color(0xFFD3D3D3),
  'darkgray': Color(0xFFA9A9A9),
  'darkgrey': Color(0xFFA9A9A9),
  'coral': Color(0xFFFF7F50),
  'tomato': Color(0xFFFF6347),
  'salmon': Color(0xFFFA8072),
  'gold': Color(0xFFFFD700),
  'khaki': Color(0xFFF0E68C),
  'pink': Color(0xFFFFC0CB),
  'brown': Color(0xFFA52A2A),
  'crimson': Color(0xFFDC143C),
  'indianred': Color(0xFFCD5C5C),
  'steelblue': Color(0xFF4682B4),
  'royalblue': Color(0xFF4169E1),
  'cornflowerblue': Color(0xFF6495ED),
  'midnightblue': Color(0xFF191970),
  'darkblue': Color(0xFF00008B),
  'darkgreen': Color(0xFF006400),
  'darkred': Color(0xFF8B0000),
  'whitesmoke': Color(0xFFF5F5F5),
  'aliceblue': Color(0xFFF0F8FF),
  'ghostwhite': Color(0xFFF8F8FF),
  'linen': Color(0xFFFAF0E6),
  'beige': Color(0xFFF5F5DC),
  'ivory': Color(0xFFFFFFF0),
  'honeydew': Color(0xFFF0FFF0),
  'mintcream': Color(0xFFF5FFFA),
  'azure': Color(0xFFF0FFFF),
  'lavender': Color(0xFFE6E6FA),
  'lightblue': Color(0xFFADD8E6),
  'lightyellow': Color(0xFFFFFFE0),
  'lightgreen': Color(0xFF90EE90),
  'lightpink': Color(0xFFFFB6C1),
};
