/// Flutter implementation of TextMeasurer.
///
/// Uses TextPainter to measure text and break it into lines,
/// bridging the pure-Dart layout engine to Flutter's text shaping.

import 'dart:ui' as ui;
import 'package:flutter/painting.dart';
import '../engine/layout.dart';

class FlutterTextMeasurer implements TextMeasurer {
  @override
  TextMetrics measureText(
    String text, {
    required double fontSize,
    required String fontFamily,
    required String fontWeight,
    required String fontStyle,
    required double maxWidth,
  }) {
    if (text.isEmpty) {
      return TextMetrics(0, 0, []);
    }

    final style = TextStyle(
      fontSize: fontSize,
      fontFamily: _mapFontFamily(fontFamily),
      fontWeight: _mapFontWeight(fontWeight),
      fontStyle: _mapFontStyle(fontStyle),
      height: 1.4,
    );

    final painter = TextPainter(
      text: TextSpan(text: text, style: style),
      textDirection: ui.TextDirection.ltr,
      maxLines: null,
    );

    painter.layout(maxWidth: maxWidth > 0 ? maxWidth : double.infinity);

    // Extract line metrics.
    final lineMetrics = painter.computeLineMetrics();
    final lines = <TextLine>[];

    int charOffset = 0;
    for (final metric in lineMetrics) {
      // Approximate which text is on each line.
      final lineEnd = painter.getPositionForOffset(
        Offset(maxWidth, metric.baseline),
      ).offset;

      final lineText = charOffset < text.length
          ? text.substring(
              charOffset,
              lineEnd.clamp(charOffset, text.length),
            )
          : '';

      lines.add(TextLine(
        lineText,
        metric.width,
        metric.height,
        metric.baseline,
      ));

      charOffset = lineEnd;
    }

    // Fallback if no line metrics available.
    if (lines.isEmpty) {
      lines.add(TextLine(text, painter.width, painter.height, fontSize));
    }

    painter.dispose();

    return TextMetrics(painter.width, painter.height, lines);
  }

  String _mapFontFamily(String family) {
    switch (family.toLowerCase()) {
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

  FontWeight _mapFontWeight(String weight) {
    switch (weight.toLowerCase()) {
      case 'bold':
      case '700':
        return FontWeight.bold;
      case '100':
        return FontWeight.w100;
      case '200':
        return FontWeight.w200;
      case '300':
        return FontWeight.w300;
      case '500':
        return FontWeight.w500;
      case '600':
        return FontWeight.w600;
      case '800':
        return FontWeight.w800;
      case '900':
        return FontWeight.w900;
      default:
        return FontWeight.normal;
    }
  }

  FontStyle _mapFontStyle(String style) {
    return style.toLowerCase() == 'italic' ? FontStyle.italic : FontStyle.normal;
  }
}
