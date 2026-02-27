/// Dark Mode — built-in plugin that injects dark-mode CSS into every page.
///
/// Works by appending a <style> element to the DOM after parsing,
/// which then flows through the normal style-computation pipeline.
/// Uses !important to override page styles.

import '../plugin.dart';
import '../../engine/dom.dart';

class DarkModePlugin extends Plugin {
  @override
  String get name => 'dark_mode';

  static const _darkCss = '''
html, body {
  background-color: #1a1a2e !important;
  color: #e0e0e0 !important;
}
a {
  color: #7ec8e3 !important;
}
img {
  opacity: 0.85;
}
* {
  border-color: #333355 !important;
}
input, textarea, select, button {
  background-color: #16213e !important;
  color: #e0e0e0 !important;
  border-color: #0f3460 !important;
}
table, th, td {
  border-color: #333355 !important;
}
pre, code, kbd, samp {
  background-color: #16213e !important;
  color: #c8d6e5 !important;
}
''';

  @override
  void onDomReady(Document document) {
    // Inject a <style> element with dark-mode rules.
    final style = Element('style');
    style.appendChild(Text(_darkCss));

    final head = document.head;
    if (head != null) {
      head.appendChild(style);
    } else {
      // Fallback: append to document element.
      document.documentElement?.appendChild(style);
    }
  }
}
