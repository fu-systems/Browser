/// Browser shell — the main UI widget that ties everything together.
///
/// Address bar, back/forward/reload, tab bar, and the rendered page content.
/// Orchestrates the pipeline: URL → fetch → parse → style → layout → paint.

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../engine/html_parser.dart';
import '../engine/css.dart';
import '../engine/style.dart';
import '../engine/layout.dart';
import '../network/fetcher.dart';
import 'flutter_text_measurer.dart';
import 'page_painter.dart';
import 'tab.dart';

class BrowserShell extends StatefulWidget {
  const BrowserShell({super.key});

  @override
  State<BrowserShell> createState() => _BrowserShellState();
}

class _BrowserShellState extends State<BrowserShell> {
  final List<Tab> _tabs = [Tab()];
  int _activeTabIndex = 0;
  final TextEditingController _addressController = TextEditingController();
  final FocusNode _addressFocus = FocusNode();
  final FlutterTextMeasurer _textMeasurer = FlutterTextMeasurer();

  Tab get _activeTab => _tabs[_activeTabIndex];

  @override
  void dispose() {
    _addressController.dispose();
    _addressFocus.dispose();
    super.dispose();
  }

  // ── Navigation ──────────────────────────────────────────────────

  Future<void> _navigate(String input) async {
    if (input.trim().isEmpty) return;

    // Normalize the URL.
    String url = input.trim();
    if (!url.startsWith('http://') && !url.startsWith('https://')) {
      url = 'https://$url';
    }

    _activeTab.navigateTo(url);
    _addressController.text = url;

    await _loadPage(url);
  }

  Future<void> _loadPage(String url) async {
    setState(() {
      _activeTab.isLoading = true;
      _activeTab.errorMessage = null;
    });

    try {
      // 1. Fetch the HTML.
      final response = await Fetcher.fetch(url);
      if (!response.isOk) {
        throw Exception('HTTP ${response.statusCode}');
      }

      // Update URL after redirects.
      _activeTab.url = response.url;
      _addressController.text = response.url;

      // 2. Parse HTML → DOM.
      final document = HtmlParser.parse(response.body);

      // 3. Extract and parse CSS.
      final stylesheets = <Stylesheet>[];

      // Internal <style> blocks.
      final internalCss = document.internalCSS;
      if (internalCss.isNotEmpty) {
        stylesheets.add(CssParser.parse(internalCss));
      }

      // External stylesheets — fetch each one.
      for (final href in document.externalStylesheetUrls) {
        try {
          final cssUrl = Fetcher.resolveUrl(response.url, href);
          final cssResponse = await Fetcher.fetch(cssUrl);
          if (cssResponse.isOk) {
            stylesheets.add(CssParser.parse(cssResponse.body));
          }
        } catch (_) {
          // Skip broken stylesheets.
        }
      }

      // 4. Compute styles.
      final body = document.body ?? document.documentElement ?? document;
      final styledTree = computeStyles(body, stylesheets);

      // 5. Layout.
      // We use the viewport width from the widget's constraints.
      final viewportWidth = _viewportWidth;
      final layoutRoot = layoutTree(styledTree, viewportWidth, _textMeasurer);

      // 6. Update tab state.
      setState(() {
        _activeTab.title = document.title.isNotEmpty
            ? document.title
            : _activeTab.url;
        _activeTab.layoutRoot = layoutRoot;
        _activeTab.pageHeight = _computePageHeight(layoutRoot);
        _activeTab.isLoading = false;
      });
    } catch (e) {
      setState(() {
        _activeTab.isLoading = false;
        _activeTab.errorMessage = e.toString();
        _activeTab.layoutRoot = null;
      });
    }
  }

  double get _viewportWidth {
    final mq = MediaQuery.of(context);
    return mq.size.width - 16; // Small margin.
  }

  double _computePageHeight(LayoutBox root) {
    double maxY = 0;
    for (final box in root.allBoxes) {
      final bottom = box.marginBox.y + box.marginBox.height;
      if (bottom > maxY) maxY = bottom;
    }
    return maxY;
  }

  void _goBack() {
    final url = _activeTab.goBack();
    if (url != null) {
      _addressController.text = url;
      _loadPage(url);
    }
  }

  void _goForward() {
    final url = _activeTab.goForward();
    if (url != null) {
      _addressController.text = url;
      _loadPage(url);
    }
  }

  void _reload() {
    if (_activeTab.url.isNotEmpty) {
      _loadPage(_activeTab.url);
    }
  }

  void _onLinkTap(String href) {
    final resolved = Fetcher.resolveUrl(_activeTab.url, href);
    _navigate(resolved);
  }

  // ── Tab management ──────────────────────────────────────────────

  void _addTab() {
    setState(() {
      _tabs.add(Tab());
      _activeTabIndex = _tabs.length - 1;
      _addressController.text = '';
    });
    _addressFocus.requestFocus();
  }

  void _closeTab(int index) {
    if (_tabs.length <= 1) return; // Keep at least one tab.
    setState(() {
      _tabs.removeAt(index);
      if (_activeTabIndex >= _tabs.length) {
        _activeTabIndex = _tabs.length - 1;
      }
      _addressController.text = _activeTab.url;
    });
  }

  void _selectTab(int index) {
    setState(() {
      _activeTabIndex = index;
      _addressController.text = _activeTab.url;
    });
  }

  // ── Scroll handling ─────────────────────────────────────────────

  void _onScroll(double delta) {
    setState(() {
      _activeTab.scrollOffset = (_activeTab.scrollOffset + delta).clamp(
        0.0,
        (_activeTab.pageHeight - 400).clamp(0.0, double.infinity),
      );
    });
  }

  // ── Link hit testing ────────────────────────────────────────────

  void _onTapPage(Offset position) {
    if (_activeTab.layoutRoot == null) return;

    final adjustedY = position.dy + _activeTab.scrollOffset;
    final href = _hitTestLink(_activeTab.layoutRoot!, position.dx, adjustedY);
    if (href != null) {
      _onLinkTap(href);
    }
  }

  String? _hitTestLink(LayoutBox box, double x, double y) {
    // Check children first (they're on top).
    for (final child in box.children.reversed) {
      final result = _hitTestLink(child, x, y);
      if (result != null) return result;
    }

    // Check this box.
    if (box.linkHref != null && box.linkHref!.isNotEmpty) {
      final r = box.content;
      if (x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height) {
        return box.linkHref;
      }
    }

    return null;
  }

  // ── Keyboard shortcuts ───────────────────────────────────────────

  KeyEventResult _handleKeyEvent(FocusNode node, KeyEvent event) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;

    final ctrl = HardwareKeyboard.instance.isControlPressed;

    // Ctrl+L — focus address bar.
    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyL) {
      _addressFocus.requestFocus();
      _addressController.selection = TextSelection(
        baseOffset: 0,
        extentOffset: _addressController.text.length,
      );
      return KeyEventResult.handled;
    }

    // Ctrl+T — new tab.
    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyT) {
      _addTab();
      return KeyEventResult.handled;
    }

    // Ctrl+W — close tab.
    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyW) {
      _closeTab(_activeTabIndex);
      return KeyEventResult.handled;
    }

    // F5 — reload.
    if (event.logicalKey == LogicalKeyboardKey.f5) {
      _reload();
      return KeyEventResult.handled;
    }

    // Alt+Left — back.
    if (HardwareKeyboard.instance.isAltPressed &&
        event.logicalKey == LogicalKeyboardKey.arrowLeft) {
      _goBack();
      return KeyEventResult.handled;
    }

    // Alt+Right — forward.
    if (HardwareKeyboard.instance.isAltPressed &&
        event.logicalKey == LogicalKeyboardKey.arrowRight) {
      _goForward();
      return KeyEventResult.handled;
    }

    // Page Down / Space (when not in address bar) — scroll down.
    if (event.logicalKey == LogicalKeyboardKey.pageDown ||
        (event.logicalKey == LogicalKeyboardKey.space && !_addressFocus.hasFocus)) {
      _onScroll(300);
      return KeyEventResult.handled;
    }

    // Page Up — scroll up.
    if (event.logicalKey == LogicalKeyboardKey.pageUp) {
      _onScroll(-300);
      return KeyEventResult.handled;
    }

    // Home — scroll to top.
    if (event.logicalKey == LogicalKeyboardKey.home && !_addressFocus.hasFocus) {
      setState(() => _activeTab.scrollOffset = 0);
      return KeyEventResult.handled;
    }

    // End — scroll to bottom.
    if (event.logicalKey == LogicalKeyboardKey.end && !_addressFocus.hasFocus) {
      setState(() {
        _activeTab.scrollOffset =
            (_activeTab.pageHeight - 400).clamp(0.0, double.infinity);
      });
      return KeyEventResult.handled;
    }

    return KeyEventResult.ignored;
  }

  // ── Build ───────────────────────────────────────────────────────

  @override
  Widget build(BuildContext context) {
    return Focus(
      onKeyEvent: _handleKeyEvent,
      autofocus: true,
      child: Scaffold(
        backgroundColor: Colors.white,
        body: Column(
          children: [
            _buildTabBar(),
            _buildToolbar(),
            Expanded(child: _buildContent()),
          ],
        ),
      ),
    );
  }

  Widget _buildTabBar() {
    return Container(
      height: 36,
      color: const Color(0xFFE8E8E8),
      child: Row(
        children: [
          Expanded(
            child: ListView.builder(
              scrollDirection: Axis.horizontal,
              itemCount: _tabs.length,
              itemBuilder: (context, index) {
                final tab = _tabs[index];
                final isActive = index == _activeTabIndex;
                return GestureDetector(
                  onTap: () => _selectTab(index),
                  child: Container(
                    constraints: const BoxConstraints(maxWidth: 200),
                    padding: const EdgeInsets.symmetric(horizontal: 12),
                    decoration: BoxDecoration(
                      color: isActive ? Colors.white : const Color(0xFFD8D8D8),
                      border: Border(
                        right: BorderSide(color: Colors.grey.shade400, width: 0.5),
                        bottom: isActive
                            ? BorderSide.none
                            : BorderSide(color: Colors.grey.shade400, width: 0.5),
                      ),
                    ),
                    child: Row(
                      children: [
                        if (tab.isLoading)
                          const Padding(
                            padding: EdgeInsets.only(right: 6),
                            child: SizedBox(
                              width: 12,
                              height: 12,
                              child: CircularProgressIndicator(strokeWidth: 1.5),
                            ),
                          ),
                        Expanded(
                          child: Text(
                            tab.title,
                            overflow: TextOverflow.ellipsis,
                            style: TextStyle(
                              fontSize: 12,
                              color: isActive ? Colors.black : Colors.grey.shade700,
                            ),
                          ),
                        ),
                        if (_tabs.length > 1)
                          GestureDetector(
                            onTap: () => _closeTab(index),
                            child: Padding(
                              padding: const EdgeInsets.only(left: 6),
                              child: Icon(
                                Icons.close,
                                size: 14,
                                color: Colors.grey.shade600,
                              ),
                            ),
                          ),
                      ],
                    ),
                  ),
                );
              },
            ),
          ),
          IconButton(
            icon: const Icon(Icons.add, size: 18),
            onPressed: _addTab,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
            tooltip: 'New Tab',
          ),
        ],
      ),
    );
  }

  Widget _buildToolbar() {
    return Container(
      height: 44,
      padding: const EdgeInsets.symmetric(horizontal: 8),
      decoration: BoxDecoration(
        color: const Color(0xFFF5F5F5),
        border: Border(bottom: BorderSide(color: Colors.grey.shade300)),
      ),
      child: Row(
        children: [
          // Back button.
          IconButton(
            icon: const Icon(Icons.arrow_back, size: 20),
            onPressed: _activeTab.canGoBack ? _goBack : null,
            tooltip: 'Back',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
          // Forward button.
          IconButton(
            icon: const Icon(Icons.arrow_forward, size: 20),
            onPressed: _activeTab.canGoForward ? _goForward : null,
            tooltip: 'Forward',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
          // Reload button.
          IconButton(
            icon: Icon(
              _activeTab.isLoading ? Icons.close : Icons.refresh,
              size: 20,
            ),
            onPressed: _reload,
            tooltip: _activeTab.isLoading ? 'Stop' : 'Reload',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
          const SizedBox(width: 8),
          // Address bar.
          Expanded(
            child: Container(
              height: 30,
              decoration: BoxDecoration(
                color: Colors.white,
                borderRadius: BorderRadius.circular(15),
                border: Border.all(color: Colors.grey.shade300),
              ),
              child: TextField(
                controller: _addressController,
                focusNode: _addressFocus,
                style: const TextStyle(fontSize: 13),
                decoration: const InputDecoration(
                  contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 0),
                  border: InputBorder.none,
                  hintText: 'Enter URL...',
                  hintStyle: TextStyle(color: Colors.grey, fontSize: 13),
                  isDense: true,
                ),
                onSubmitted: _navigate,
                textInputAction: TextInputAction.go,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildContent() {
    if (_activeTab.errorMessage != null) {
      return _buildErrorPage();
    }

    if (_activeTab.layoutRoot == null && !_activeTab.isLoading) {
      return _buildStartPage();
    }

    if (_activeTab.isLoading && _activeTab.layoutRoot == null) {
      return const Center(child: CircularProgressIndicator());
    }

    return Listener(
      onPointerSignal: (event) {
        if (event is PointerScrollEvent) {
          _onScroll(event.scrollDelta.dy);
        }
      },
      child: GestureDetector(
        onTap: () {
          // We need the tap position — use onTapDown instead.
        },
        onTapDown: (details) {
          _onTapPage(details.localPosition);
        },
        child: ClipRect(
          child: CustomPaint(
            painter: PagePainter(
              rootBox: _activeTab.layoutRoot,
              scrollOffset: _activeTab.scrollOffset,
            ),
            size: Size.infinite,
          ),
        ),
      ),
    );
  }

  Widget _buildStartPage() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            'Pane',
            style: TextStyle(
              fontSize: 48,
              fontWeight: FontWeight.w300,
              color: Colors.grey.shade700,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'A minimal, secure web browser',
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey.shade500,
            ),
          ),
          const SizedBox(height: 24),
          Text(
            'Type a URL above and press Enter',
            style: TextStyle(
              fontSize: 13,
              color: Colors.grey.shade400,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildErrorPage() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.error_outline, size: 48, color: Colors.red.shade300),
            const SizedBox(height: 16),
            const Text(
              'Failed to load page',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w500),
            ),
            const SizedBox(height: 8),
            Text(
              _activeTab.errorMessage!,
              style: TextStyle(fontSize: 13, color: Colors.grey.shade600),
              textAlign: TextAlign.center,
            ),
            const SizedBox(height: 16),
            TextButton(
              onPressed: _reload,
              child: const Text('Try again'),
            ),
          ],
        ),
      ),
    );
  }
}
