/// Page painter — renders the layout tree onto a Flutter Canvas.
///
/// Walks the LayoutBox tree and draws backgrounds, borders, text,
/// images, and form element placeholders using Flutter's Canvas API.
/// Supports border-radius, box-shadow, opacity, gradients, and
/// the full CSS color space (named, hex, rgb, rgba, hsl, hsla).

import 'dart:ui' as ui;
import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../engine/layout.dart' as engine;
import '../engine/style.dart';
import '../engine/dom.dart' as dom;
import '../plugin/plugin_pipeline.dart';

class PagePainter extends CustomPainter {
  final engine.LayoutBox? rootBox;
  final double scrollOffset;
  final Map<String, ui.Image> imageCache;
  final String searchQuery;
  final int currentSearchIndex;
  final List<engine.Rect> searchRects;
  final PluginPipeline? pluginPipeline;

  PagePainter({
    this.rootBox,
    this.scrollOffset = 0,
    this.imageCache = const {},
    this.searchQuery = '',
    this.currentSearchIndex = -1,
    this.searchRects = const [],
    this.pluginPipeline,
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

    // Plugin paint overlays.
    pluginPipeline?.runPaint(canvas, size, scrollOffset);

    canvas.restore();
  }

  void _paintBox(Canvas canvas, engine.LayoutBox box) {
    // Handle opacity.
    final hasOpacity = box.opacity < 1.0;
    if (hasOpacity) {
      canvas.saveLayer(
        ui.Rect.fromLTWH(
          box.marginBox.x, box.marginBox.y,
          box.marginBox.width, box.marginBox.height,
        ),
        Paint()..color = Color.fromARGB((box.opacity * 255).round(), 255, 255, 255),
      );
    }

    // Handle overflow: hidden (clip children).
    final shouldClip = box.overflow == 'hidden' || box.overflow == 'scroll' || box.overflow == 'auto';
    if (shouldClip) {
      canvas.save();
      canvas.clipRect(ui.Rect.fromLTWH(
        box.paddingBox.x, box.paddingBox.y,
        box.paddingBox.width, box.paddingBox.height,
      ));
    }

    _paintBoxShadow(canvas, box);
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

    if (shouldClip) {
      canvas.restore();
    }

    if (hasOpacity) {
      canvas.restore();
    }
  }

  void _paintBoxShadow(Canvas canvas, engine.LayoutBox box) {
    if (box.boxShadow == null) return;

    final shadows = _parseBoxShadows(box.boxShadow!);
    for (final shadow in shadows) {
      if (shadow.inset) continue; // Skip inset shadows for now.

      final rect = box.borderBox;
      final shadowRect = ui.Rect.fromLTWH(
        rect.x + shadow.offsetX,
        rect.y + shadow.offsetY,
        rect.width + shadow.spread * 2,
        rect.height + shadow.spread * 2,
      ).translate(-shadow.spread, -shadow.spread);

      final paint = Paint()
        ..color = shadow.color
        ..maskFilter = shadow.blur > 0
            ? MaskFilter.blur(BlurStyle.normal, shadow.blur / 2)
            : null;

      if (_hasRoundedCorners(box)) {
        canvas.drawRRect(
          RRect.fromRectAndCorners(
            shadowRect,
            topLeft: Radius.circular(box.borderRadiusTL),
            topRight: Radius.circular(box.borderRadiusTR),
            bottomRight: Radius.circular(box.borderRadiusBR),
            bottomLeft: Radius.circular(box.borderRadiusBL),
          ),
          paint,
        );
      } else {
        canvas.drawRect(shadowRect, paint);
      }
    }
  }

  void _paintBackground(Canvas canvas, engine.LayoutBox box) {
    final bgStr = box.styledNode?['background-color'] ?? box.styledNode?['background'] ?? '';
    if (bgStr.isEmpty) return;

    final rect = box.paddingBox;
    final uiRect = ui.Rect.fromLTWH(rect.x, rect.y, rect.width, rect.height);

    // Check for gradient.
    if (bgStr.contains('gradient')) {
      final gradient = _parseGradient(bgStr, uiRect);
      if (gradient != null) {
        final paint = Paint()..shader = gradient;
        if (_hasRoundedCorners(box)) {
          canvas.drawRRect(_makeRRect(box, rect), paint);
        } else {
          canvas.drawRect(uiRect, paint);
        }
        return;
      }
    }

    final bgColor = _resolveColor(bgStr);
    if (bgColor == null) return;

    final paint = Paint()..color = bgColor;
    if (_hasRoundedCorners(box)) {
      canvas.drawRRect(_makeRRect(box, rect), paint);
    } else {
      canvas.drawRect(uiRect, paint);
    }
  }

  void _paintBorders(Canvas canvas, engine.LayoutBox box) {
    final bw = box.border;
    if (bw.top == 0 && bw.right == 0 && bw.bottom == 0 && bw.left == 0) return;

    final borderColor = _resolveBorderColor(box.styledNode);
    final rect = box.borderBox;

    if (_hasRoundedCorners(box)) {
      // Draw rounded border.
      final rrect = _makeRRect(box, rect);
      final paint = Paint()
        ..color = borderColor
        ..style = PaintingStyle.stroke
        ..strokeWidth = math.max(bw.top, math.max(bw.right, math.max(bw.bottom, bw.left)));
      canvas.drawRRect(rrect, paint);
      return;
    }

    final paint = Paint()
      ..color = borderColor
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;

    // Parse individual border colors.
    final topColor = _resolveBorderSideColor(box.styledNode, 'border-top') ?? borderColor;
    final rightColor = _resolveBorderSideColor(box.styledNode, 'border-right') ?? borderColor;
    final bottomColor = _resolveBorderSideColor(box.styledNode, 'border-bottom') ?? borderColor;
    final leftColor = _resolveBorderSideColor(box.styledNode, 'border-left') ?? borderColor;

    if (bw.top > 0) {
      paint.strokeWidth = bw.top;
      paint.color = topColor;
      canvas.drawLine(Offset(rect.x, rect.y), Offset(rect.x + rect.width, rect.y), paint);
    }
    if (bw.right > 0) {
      paint.strokeWidth = bw.right;
      paint.color = rightColor;
      canvas.drawLine(Offset(rect.x + rect.width, rect.y), Offset(rect.x + rect.width, rect.y + rect.height), paint);
    }
    if (bw.bottom > 0) {
      paint.strokeWidth = bw.bottom;
      paint.color = bottomColor;
      canvas.drawLine(Offset(rect.x, rect.y + rect.height), Offset(rect.x + rect.width, rect.y + rect.height), paint);
    }
    if (bw.left > 0) {
      paint.strokeWidth = bw.left;
      paint.color = leftColor;
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

      if (_hasRoundedCorners(box)) {
        canvas.save();
        canvas.clipRRect(_makeRRect(box, r));
        canvas.drawImageRect(img, src, dst, Paint());
        canvas.restore();
      } else {
        canvas.drawImageRect(img, src, dst, Paint());
      }
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

    if (type == 'range') {
      // Range slider.
      final trackY = r.y + r.height / 2;
      canvas.drawLine(
        Offset(r.x, trackY),
        Offset(r.x + r.width, trackY),
        Paint()..color = Colors.grey.shade300..strokeWidth = 4..strokeCap = StrokeCap.round,
      );
      canvas.drawCircle(
        Offset(r.x + r.width / 2, trackY), 8,
        Paint()..color = Colors.blue,
      );
      return;
    }

    if (type == 'color') {
      final rect = ui.Rect.fromLTWH(r.x, r.y, r.width, r.height);
      final colorValue = box.formValue?.isNotEmpty == true ? box.formValue! : '#000000';
      final color = _resolveColor(colorValue) ?? Colors.black;
      canvas.drawRect(rect, Paint()..color = color);
      canvas.drawRect(rect, Paint()..color = Colors.grey..style = PaintingStyle.stroke..strokeWidth = 1);
      return;
    }

    if (tag == 'progress') {
      final rrect = RRect.fromRectAndRadius(
        ui.Rect.fromLTWH(r.x, r.y, r.width, r.height),
        const Radius.circular(4),
      );
      canvas.drawRRect(rrect, Paint()..color = Colors.grey.shade200);
      final progress = 0.5; // Default 50% for display.
      final fillRrect = RRect.fromRectAndRadius(
        ui.Rect.fromLTWH(r.x, r.y, r.width * progress, r.height),
        const Radius.circular(4),
      );
      canvas.drawRRect(fillRrect, Paint()..color = Colors.blue);
      return;
    }

    if (tag == 'meter') {
      final rrect = RRect.fromRectAndRadius(
        ui.Rect.fromLTWH(r.x, r.y, r.width, r.height),
        const Radius.circular(4),
      );
      canvas.drawRRect(rrect, Paint()..color = Colors.grey.shade200);
      final fillRrect = RRect.fromRectAndRadius(
        ui.Rect.fromLTWH(r.x, r.y, r.width * 0.6, r.height),
        const Radius.circular(4),
      );
      canvas.drawRRect(fillRrect, Paint()..color = Colors.green);
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
    final textShadow = styled?['text-shadow'] ?? '';

    TextDecoration textDecoration = TextDecoration.none;
    if (decoration.contains('underline')) {
      textDecoration = TextDecoration.underline;
    } else if (decoration.contains('line-through')) {
      textDecoration = TextDecoration.lineThrough;
    } else if (decoration.contains('overline')) {
      textDecoration = TextDecoration.overline;
    }

    // Map font-weight values.
    FontWeight fw = FontWeight.normal;
    switch (fontWeight) {
      case 'bold':
      case '700':
        fw = FontWeight.bold;
      case '100':
        fw = FontWeight.w100;
      case '200':
        fw = FontWeight.w200;
      case '300':
        fw = FontWeight.w300;
      case '400':
        fw = FontWeight.w400;
      case '500':
        fw = FontWeight.w500;
      case '600':
        fw = FontWeight.w600;
      case '800':
        fw = FontWeight.w800;
      case '900':
        fw = FontWeight.w900;
    }

    // Parse text shadow.
    List<Shadow>? shadows;
    if (textShadow.isNotEmpty && textShadow != 'none') {
      shadows = _parseTextShadows(textShadow);
    }

    final style = TextStyle(
      fontSize: fontSize,
      fontFamily: _mapFontFamily(fontFamily),
      fontWeight: fw,
      fontStyle: fontStyle == 'italic' ? FontStyle.italic : FontStyle.normal,
      color: color,
      decoration: textDecoration,
      decorationColor: color,
      height: 1.4,
      shadows: shadows,
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

  // ── Helper methods ────────────────────────────────────────────────

  bool _hasRoundedCorners(engine.LayoutBox box) =>
      box.borderRadiusTL > 0 || box.borderRadiusTR > 0 ||
      box.borderRadiusBR > 0 || box.borderRadiusBL > 0;

  RRect _makeRRect(engine.LayoutBox box, engine.Rect rect) {
    return RRect.fromRectAndCorners(
      ui.Rect.fromLTWH(rect.x, rect.y, rect.width, rect.height),
      topLeft: Radius.circular(box.borderRadiusTL),
      topRight: Radius.circular(box.borderRadiusTR),
      bottomRight: Radius.circular(box.borderRadiusBR),
      bottomLeft: Radius.circular(box.borderRadiusBL),
    );
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

  Color? _resolveBorderSideColor(StyledNode? styled, String side) {
    if (styled == null) return null;
    final sideColor = styled['$side-color'];
    if (sideColor != null) return _resolveColor(sideColor);
    final sideVal = styled[side];
    if (sideVal != null && sideVal.isNotEmpty) {
      final parts = sideVal.split(RegExp(r'\s+'));
      if (parts.length >= 3) return _resolveColor(parts[2]);
    }
    return null;
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

// ── Box shadow parsing ──────────────────────────────────────────────

class _BoxShadow {
  final double offsetX;
  final double offsetY;
  final double blur;
  final double spread;
  final Color color;
  final bool inset;
  _BoxShadow({
    this.offsetX = 0, this.offsetY = 0, this.blur = 0, this.spread = 0,
    this.color = const Color(0xFF000000), this.inset = false,
  });
}

List<_BoxShadow> _parseBoxShadows(String value) {
  final shadows = <_BoxShadow>[];
  // Simple split on comma — doesn't handle commas inside color functions perfectly
  // but works for the vast majority of cases.
  final parts = value.split(RegExp(r',\s*(?![^(]*\))'));

  for (final part in parts) {
    final trimmed = part.trim();
    if (trimmed.isEmpty || trimmed == 'none') continue;

    bool inset = trimmed.startsWith('inset');
    final working = inset ? trimmed.substring(5).trim() : trimmed;

    // Extract color first (may appear at beginning or end).
    Color? color;
    String remaining = working;

    // Try to find a color at the end.
    final rgbMatch = RegExp(r'(rgba?\([^)]+\))\s*$').firstMatch(remaining);
    final hslMatch = RegExp(r'(hsla?\([^)]+\))\s*$').firstMatch(remaining);
    if (rgbMatch != null) {
      color = _resolveColor(rgbMatch.group(1)!);
      remaining = remaining.substring(0, rgbMatch.start).trim();
    } else if (hslMatch != null) {
      color = _resolveColor(hslMatch.group(1)!);
      remaining = remaining.substring(0, hslMatch.start).trim();
    } else {
      // Try color at end as named/hex.
      final tokens = remaining.split(RegExp(r'\s+'));
      if (tokens.isNotEmpty) {
        final maybeColor = _resolveColor(tokens.last);
        if (maybeColor != null && tokens.length > 2) {
          color = maybeColor;
          remaining = tokens.sublist(0, tokens.length - 1).join(' ');
        }
      }
    }

    color ??= const Color(0x33000000);

    // Parse numeric values.
    final nums = remaining.split(RegExp(r'\s+')).map((s) => _parsePx(s)).toList();
    shadows.add(_BoxShadow(
      offsetX: nums.isNotEmpty ? nums[0] : 0,
      offsetY: nums.length > 1 ? nums[1] : 0,
      blur: nums.length > 2 ? nums[2] : 0,
      spread: nums.length > 3 ? nums[3] : 0,
      color: color,
      inset: inset,
    ));
  }
  return shadows;
}

// ── Text shadow parsing ─────────────────────────────────────────────

List<Shadow> _parseTextShadows(String value) {
  final shadows = <Shadow>[];
  final parts = value.split(RegExp(r',\s*(?![^(]*\))'));

  for (final part in parts) {
    final trimmed = part.trim();
    if (trimmed.isEmpty || trimmed == 'none') continue;

    Color? color;
    String remaining = trimmed;

    // Extract color.
    final rgbMatch = RegExp(r'(rgba?\([^)]+\))\s*').firstMatch(remaining);
    final hslMatch = RegExp(r'(hsla?\([^)]+\))\s*').firstMatch(remaining);
    if (rgbMatch != null) {
      color = _resolveColor(rgbMatch.group(1)!);
      remaining = remaining.replaceFirst(rgbMatch.group(0)!, '').trim();
    } else if (hslMatch != null) {
      color = _resolveColor(hslMatch.group(1)!);
      remaining = remaining.replaceFirst(hslMatch.group(0)!, '').trim();
    } else {
      final tokens = remaining.split(RegExp(r'\s+'));
      for (int i = tokens.length - 1; i >= 0; i--) {
        final c = _resolveColor(tokens[i]);
        if (c != null) {
          color = c;
          tokens.removeAt(i);
          remaining = tokens.join(' ');
          break;
        }
      }
    }

    color ??= const Color(0xFF000000);
    final nums = remaining.split(RegExp(r'\s+')).map((s) => _parsePx(s)).toList();

    shadows.add(Shadow(
      offset: Offset(
        nums.isNotEmpty ? nums[0] : 0,
        nums.length > 1 ? nums[1] : 0,
      ),
      blurRadius: nums.length > 2 ? nums[2] : 0,
      color: color,
    ));
  }
  return shadows;
}

// ── Gradient parsing ────────────────────────────────────────────────

ui.Shader? _parseGradient(String value, ui.Rect rect) {
  // linear-gradient(direction, color1, color2, ...)
  final lgMatch = RegExp(r'linear-gradient\((.+)\)', caseSensitive: false).firstMatch(value);
  if (lgMatch != null) {
    return _parseLinearGradient(lgMatch.group(1)!, rect);
  }

  // radial-gradient.
  final rgMatch = RegExp(r'radial-gradient\((.+)\)', caseSensitive: false).firstMatch(value);
  if (rgMatch != null) {
    return _parseRadialGradient(rgMatch.group(1)!, rect);
  }

  return null;
}

ui.Shader? _parseLinearGradient(String args, ui.Rect rect) {
  final parts = _splitGradientArgs(args);
  if (parts.length < 2) return null;

  // Parse direction.
  double angle = 180; // Default: top to bottom.
  int colorStartIndex = 0;

  final first = parts[0].trim().toLowerCase();
  if (first.startsWith('to ')) {
    final dir = first.substring(3).trim();
    angle = _directionToAngle(dir);
    colorStartIndex = 1;
  } else if (first.endsWith('deg')) {
    angle = double.tryParse(first.replaceAll('deg', '')) ?? 180;
    colorStartIndex = 1;
  } else if (first.endsWith('turn')) {
    angle = (double.tryParse(first.replaceAll('turn', '')) ?? 0.5) * 360;
    colorStartIndex = 1;
  }

  // Parse color stops.
  final colors = <Color>[];
  final stops = <double>[];
  final colorParts = parts.sublist(colorStartIndex);

  for (int i = 0; i < colorParts.length; i++) {
    final cp = colorParts[i].trim();
    // Try to extract a percentage stop.
    final stopMatch = RegExp(r'(.+?)\s+(\d+(?:\.\d+)?%)\s*$').firstMatch(cp);
    if (stopMatch != null) {
      final c = _resolveColor(stopMatch.group(1)!.trim());
      if (c != null) {
        colors.add(c);
        stops.add(double.parse(stopMatch.group(2)!.replaceAll('%', '')) / 100);
      }
    } else {
      final c = _resolveColor(cp);
      if (c != null) {
        colors.add(c);
        stops.add(i / math.max(1, colorParts.length - 1));
      }
    }
  }

  if (colors.length < 2) return null;

  // Convert angle to start/end points.
  final rad = (angle - 90) * math.pi / 180;
  final cx = rect.center.dx;
  final cy = rect.center.dy;
  final len = math.max(rect.width, rect.height) / 2;
  final start = Offset(cx - math.cos(rad) * len, cy - math.sin(rad) * len);
  final end = Offset(cx + math.cos(rad) * len, cy + math.sin(rad) * len);

  return ui.Gradient.linear(start, end, colors, stops);
}

ui.Shader? _parseRadialGradient(String args, ui.Rect rect) {
  final parts = _splitGradientArgs(args);
  if (parts.length < 2) return null;

  // Parse color stops (skip shape/position for simplicity).
  final colors = <Color>[];
  final stops = <double>[];

  for (int i = 0; i < parts.length; i++) {
    final cp = parts[i].trim();
    final c = _resolveColor(cp);
    if (c != null) {
      colors.add(c);
      stops.add(colors.length == 1 ? 0.0 : 1.0);
    }
  }

  if (colors.length < 2) return null;
  // Recalculate stops evenly.
  for (int i = 0; i < stops.length; i++) {
    stops[i] = i / (stops.length - 1);
  }

  return ui.Gradient.radial(
    rect.center,
    math.max(rect.width, rect.height) / 2,
    colors,
    stops,
  );
}

List<String> _splitGradientArgs(String args) {
  final result = <String>[];
  int depth = 0;
  final buf = StringBuffer();
  for (int i = 0; i < args.length; i++) {
    final c = args[i];
    if (c == '(') depth++;
    if (c == ')') depth--;
    if (c == ',' && depth == 0) {
      result.add(buf.toString());
      buf.clear();
    } else {
      buf.write(c);
    }
  }
  if (buf.isNotEmpty) result.add(buf.toString());
  return result;
}

double _directionToAngle(String direction) {
  switch (direction.trim()) {
    case 'top':
      return 0;
    case 'right':
      return 90;
    case 'bottom':
      return 180;
    case 'left':
      return 270;
    case 'top right':
      return 45;
    case 'top left':
      return 315;
    case 'bottom right':
      return 135;
    case 'bottom left':
      return 225;
    default:
      return 180;
  }
}

// ── Color parsing ───────────────────────────────────────────────────

Color? _resolveColor(String value) {
  if (value.isEmpty || value == 'transparent' || value == 'none') return null;

  final trimmed = value.trim().toLowerCase();

  final named = _namedColors[trimmed];
  if (named != null) return named;

  if (trimmed.startsWith('#')) {
    final hex = trimmed.substring(1);
    if (hex.length == 3) {
      final r = int.parse(hex[0] * 2, radix: 16);
      final g = int.parse(hex[1] * 2, radix: 16);
      final b = int.parse(hex[2] * 2, radix: 16);
      return Color.fromARGB(255, r, g, b);
    }
    if (hex.length == 4) {
      final r = int.parse(hex[0] * 2, radix: 16);
      final g = int.parse(hex[1] * 2, radix: 16);
      final b = int.parse(hex[2] * 2, radix: 16);
      final a = int.parse(hex[3] * 2, radix: 16);
      return Color.fromARGB(a, r, g, b);
    }
    if (hex.length == 6) {
      final n = int.tryParse(hex, radix: 16);
      if (n != null) return Color.fromARGB(255, (n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
    }
    if (hex.length == 8) {
      final n = int.tryParse(hex, radix: 16);
      if (n != null) {
        return Color.fromARGB(
          (n >> 24) & 0xFF,
          (n >> 16) & 0xFF,
          (n >> 8) & 0xFF,
          n & 0xFF,
        );
      }
    }
  }

  // rgb() / rgba()
  final rgbMatch = RegExp(r'rgba?\(\s*(\d+)\s*[,\s]\s*(\d+)\s*[,\s]\s*(\d+)\s*(?:[,/]\s*([\d.]+%?))?\s*\)').firstMatch(trimmed);
  if (rgbMatch != null) {
    final r = int.parse(rgbMatch.group(1)!).clamp(0, 255);
    final g = int.parse(rgbMatch.group(2)!).clamp(0, 255);
    final b = int.parse(rgbMatch.group(3)!).clamp(0, 255);
    int a = 255;
    if (rgbMatch.group(4) != null) {
      final alphaStr = rgbMatch.group(4)!;
      if (alphaStr.endsWith('%')) {
        a = (double.parse(alphaStr.replaceAll('%', '')) / 100 * 255).round();
      } else {
        a = (double.parse(alphaStr) * 255).round();
      }
    }
    return Color.fromARGB(a.clamp(0, 255), r, g, b);
  }

  // hsl() / hsla()
  final hslMatch = RegExp(r'hsla?\(\s*([\d.]+)(?:deg)?\s*[,\s]\s*([\d.]+)%\s*[,\s]\s*([\d.]+)%\s*(?:[,/]\s*([\d.]+%?))?\s*\)').firstMatch(trimmed);
  if (hslMatch != null) {
    final h = double.parse(hslMatch.group(1)!) % 360;
    final s = (double.parse(hslMatch.group(2)!) / 100).clamp(0.0, 1.0);
    final l = (double.parse(hslMatch.group(3)!) / 100).clamp(0.0, 1.0);
    double a = 1.0;
    if (hslMatch.group(4) != null) {
      final alphaStr = hslMatch.group(4)!;
      if (alphaStr.endsWith('%')) {
        a = double.parse(alphaStr.replaceAll('%', '')) / 100;
      } else {
        a = double.parse(alphaStr);
      }
    }
    return _hslToColor(h, s, l, a);
  }

  // currentColor -> fallback to black.
  if (trimmed == 'currentcolor') return const Color(0xFF000000);

  return null;
}

Color _hslToColor(double h, double s, double l, double a) {
  final c = (1 - (2 * l - 1).abs()) * s;
  final x = c * (1 - ((h / 60) % 2 - 1).abs());
  final m = l - c / 2;

  double r1, g1, b1;
  if (h < 60) {
    r1 = c; g1 = x; b1 = 0;
  } else if (h < 120) {
    r1 = x; g1 = c; b1 = 0;
  } else if (h < 180) {
    r1 = 0; g1 = c; b1 = x;
  } else if (h < 240) {
    r1 = 0; g1 = x; b1 = c;
  } else if (h < 300) {
    r1 = x; g1 = 0; b1 = c;
  } else {
    r1 = c; g1 = 0; b1 = x;
  }

  return Color.fromARGB(
    (a * 255).round().clamp(0, 255),
    ((r1 + m) * 255).round().clamp(0, 255),
    ((g1 + m) * 255).round().clamp(0, 255),
    ((b1 + m) * 255).round().clamp(0, 255),
  );
}

double _parsePx(String value) {
  if (value.endsWith('px')) {
    return double.tryParse(value.replaceAll('px', '')) ?? 16;
  }
  if (value.endsWith('rem')) {
    final n = double.tryParse(value.replaceAll('rem', ''));
    if (n != null) return n * 16;
  }
  if (value.endsWith('em')) {
    final n = double.tryParse(value.replaceAll('em', ''));
    if (n != null) return n * 16;
  }
  if (value.endsWith('pt')) {
    final n = double.tryParse(value.replaceAll('pt', ''));
    if (n != null) return n * 1.333;
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
  // CSS Level 1
  'black': Color(0xFF000000),
  'white': Color(0xFFFFFFFF),
  'red': Color(0xFFFF0000),
  'green': Color(0xFF008000),
  'blue': Color(0xFF0000FF),
  'yellow': Color(0xFFFFFF00),
  'cyan': Color(0xFF00FFFF),
  'magenta': Color(0xFFFF00FF),
  'silver': Color(0xFFC0C0C0),
  'gray': Color(0xFF808080),
  'grey': Color(0xFF808080),
  'maroon': Color(0xFF800000),
  'olive': Color(0xFF808000),
  'lime': Color(0xFF00FF00),
  'aqua': Color(0xFF00FFFF),
  'teal': Color(0xFF008080),
  'navy': Color(0xFF000080),
  'fuchsia': Color(0xFFFF00FF),
  'purple': Color(0xFF800080),

  // CSS Level 2/3 extended
  'orange': Color(0xFFFFA500),
  'aliceblue': Color(0xFFF0F8FF),
  'antiquewhite': Color(0xFFFAEBD7),
  'aquamarine': Color(0xFF7FFFD4),
  'azure': Color(0xFFF0FFFF),
  'beige': Color(0xFFF5F5DC),
  'bisque': Color(0xFFFFE4C4),
  'blanchedalmond': Color(0xFFFFEBCD),
  'blueviolet': Color(0xFF8A2BE2),
  'brown': Color(0xFFA52A2A),
  'burlywood': Color(0xFFDEB887),
  'cadetblue': Color(0xFF5F9EA0),
  'chartreuse': Color(0xFF7FFF00),
  'chocolate': Color(0xFFD2691E),
  'coral': Color(0xFFFF7F50),
  'cornflowerblue': Color(0xFF6495ED),
  'cornsilk': Color(0xFFFFF8DC),
  'crimson': Color(0xFFDC143C),
  'darkblue': Color(0xFF00008B),
  'darkcyan': Color(0xFF008B8B),
  'darkgoldenrod': Color(0xFFB8860B),
  'darkgray': Color(0xFFA9A9A9),
  'darkgrey': Color(0xFFA9A9A9),
  'darkgreen': Color(0xFF006400),
  'darkkhaki': Color(0xFFBDB76B),
  'darkmagenta': Color(0xFF8B008B),
  'darkolivegreen': Color(0xFF556B2F),
  'darkorange': Color(0xFFFF8C00),
  'darkorchid': Color(0xFF9932CC),
  'darkred': Color(0xFF8B0000),
  'darksalmon': Color(0xFFE9967A),
  'darkseagreen': Color(0xFF8FBC8F),
  'darkslateblue': Color(0xFF483D8B),
  'darkslategray': Color(0xFF2F4F4F),
  'darkslategrey': Color(0xFF2F4F4F),
  'darkturquoise': Color(0xFF00CED1),
  'darkviolet': Color(0xFF9400D3),
  'deeppink': Color(0xFFFF1493),
  'deepskyblue': Color(0xFF00BFFF),
  'dimgray': Color(0xFF696969),
  'dimgrey': Color(0xFF696969),
  'dodgerblue': Color(0xFF1E90FF),
  'firebrick': Color(0xFFB22222),
  'floralwhite': Color(0xFFFFFAF0),
  'forestgreen': Color(0xFF228B22),
  'gainsboro': Color(0xFFDCDCDC),
  'ghostwhite': Color(0xFFF8F8FF),
  'gold': Color(0xFFFFD700),
  'goldenrod': Color(0xFFDAA520),
  'greenyellow': Color(0xFFADFF2F),
  'honeydew': Color(0xFFF0FFF0),
  'hotpink': Color(0xFFFF69B4),
  'indianred': Color(0xFFCD5C5C),
  'indigo': Color(0xFF4B0082),
  'ivory': Color(0xFFFFFFF0),
  'khaki': Color(0xFFF0E68C),
  'lavender': Color(0xFFE6E6FA),
  'lavenderblush': Color(0xFFFFF0F5),
  'lawngreen': Color(0xFF7CFC00),
  'lemonchiffon': Color(0xFFFFFACD),
  'lightblue': Color(0xFFADD8E6),
  'lightcoral': Color(0xFFF08080),
  'lightcyan': Color(0xFFE0FFFF),
  'lightgoldenrodyellow': Color(0xFFFAFAD2),
  'lightgray': Color(0xFFD3D3D3),
  'lightgrey': Color(0xFFD3D3D3),
  'lightgreen': Color(0xFF90EE90),
  'lightpink': Color(0xFFFFB6C1),
  'lightsalmon': Color(0xFFFFA07A),
  'lightseagreen': Color(0xFF20B2AA),
  'lightskyblue': Color(0xFF87CEFA),
  'lightslategray': Color(0xFF778899),
  'lightslategrey': Color(0xFF778899),
  'lightsteelblue': Color(0xFFB0C4DE),
  'lightyellow': Color(0xFFFFFFE0),
  'limegreen': Color(0xFF32CD32),
  'linen': Color(0xFFFAF0E6),
  'mediumaquamarine': Color(0xFF66CDAA),
  'mediumblue': Color(0xFF0000CD),
  'mediumorchid': Color(0xFFBA55D3),
  'mediumpurple': Color(0xFF9370DB),
  'mediumseagreen': Color(0xFF3CB371),
  'mediumslateblue': Color(0xFF7B68EE),
  'mediumspringgreen': Color(0xFF00FA9A),
  'mediumturquoise': Color(0xFF48D1CC),
  'mediumvioletred': Color(0xFFC71585),
  'midnightblue': Color(0xFF191970),
  'mintcream': Color(0xFFF5FFFA),
  'mistyrose': Color(0xFFFFE4E1),
  'moccasin': Color(0xFFFFE4B5),
  'navajowhite': Color(0xFFFFDEAD),
  'oldlace': Color(0xFFFDF5E6),
  'olivedrab': Color(0xFF6B8E23),
  'orangered': Color(0xFFFF4500),
  'orchid': Color(0xFFDA70D6),
  'palegoldenrod': Color(0xFFEEE8AA),
  'palegreen': Color(0xFF98FB98),
  'paleturquoise': Color(0xFFAFEEEE),
  'palevioletred': Color(0xFFDB7093),
  'papayawhip': Color(0xFFFFEFD5),
  'peachpuff': Color(0xFFFFDAB9),
  'peru': Color(0xFFCD853F),
  'pink': Color(0xFFFFC0CB),
  'plum': Color(0xFFDDA0DD),
  'powderblue': Color(0xFFB0E0E6),
  'rosybrown': Color(0xFFBC8F8F),
  'royalblue': Color(0xFF4169E1),
  'saddlebrown': Color(0xFF8B4513),
  'salmon': Color(0xFFFA8072),
  'sandybrown': Color(0xFFF4A460),
  'seagreen': Color(0xFF2E8B57),
  'seashell': Color(0xFFFFF5EE),
  'sienna': Color(0xFFA0522D),
  'skyblue': Color(0xFF87CEEB),
  'slateblue': Color(0xFF6A5ACD),
  'slategray': Color(0xFF708090),
  'slategrey': Color(0xFF708090),
  'snow': Color(0xFFFFFAFA),
  'springgreen': Color(0xFF00FF7F),
  'steelblue': Color(0xFF4682B4),
  'tan': Color(0xFFD2B48C),
  'thistle': Color(0xFFD8BFD8),
  'tomato': Color(0xFFFF6347),
  'turquoise': Color(0xFF40E0D0),
  'violet': Color(0xFFEE82EE),
  'wheat': Color(0xFFF5DEB3),
  'whitesmoke': Color(0xFFF5F5F5),
  'yellowgreen': Color(0xFF9ACD32),

  // CSS Level 4
  'rebeccapurple': Color(0xFF663399),
};
