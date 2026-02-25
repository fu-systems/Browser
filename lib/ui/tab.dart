/// Tab model — manages per-tab state.
///
/// Each tab has its own URL, page title, history stacks,
/// scroll position, and rendered layout tree.

import '../engine/layout.dart';

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
