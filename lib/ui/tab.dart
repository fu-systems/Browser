/// Tab model — manages per-tab state.
///
/// Each tab has its own URL, page title, history stacks,
/// scroll position, rendered layout tree, and Phase 2 state.

import 'dart:ui' as ui;
import '../engine/layout.dart';
import '../plugin/built_in/script_manager.dart';

class Tab {
  String url;
  String title;
  final List<String> backHistory = [];
  final List<String> forwardHistory = [];
  LayoutBox? layoutRoot;
  double scrollOffset = 0;
  double pageHeight = 0;
  bool isLoading = false;
  String? errorMessage;

  /// Phase 2: image cache (URL → decoded ui.Image).
  final Map<String, ui.Image> imageCache = {};

  /// Phase 2: raw HTML source for View Source.
  String? sourceHtml;

  /// Phase 2: search state.
  String searchQuery = '';
  List<Rect> searchRects = [];
  int searchIndex = -1;

  /// Cookie toggle — per-tab, default off.
  bool cookiesEnabled = false;

  /// JavaScript execution mode — per-tab, default off.
  ScriptMode scriptMode = ScriptMode.off;

  /// JavaScript outbound data transmission — per-tab.
  TransmitMode scriptTransmitMode = TransmitMode.normal;

  /// Execution log from the last script run on this tab.
  List<String> scriptLog = [];

  Tab({this.url = '', this.title = 'New Tab'});

  bool get canGoBack => backHistory.isNotEmpty;
  bool get canGoForward => forwardHistory.isNotEmpty;

  void navigateTo(String newUrl) {
    if (url.isNotEmpty) {
      backHistory.add(url);
    }
    forwardHistory.clear();
    url = newUrl;
    scrollOffset = 0;
  }

  String? goBack() {
    if (!canGoBack) return null;
    forwardHistory.add(url);
    url = backHistory.removeLast();
    scrollOffset = 0;
    return url;
  }

  String? goForward() {
    if (!canGoForward) return null;
    backHistory.add(url);
    url = forwardHistory.removeLast();
    scrollOffset = 0;
    return url;
  }
}
