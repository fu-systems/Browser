/// Browser shell — the main UI widget that ties everything together.
///
/// Address bar, back/forward/reload, tab bar, and the rendered page content.
/// Orchestrates the pipeline: URL → fetch → parse → style → layout → paint.
/// Phase 2 adds: images, forms, text selection, scrollbar, view source, find.

import 'dart:ui' as ui;
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../engine/html_parser.dart';
import '../engine/css.dart' as css;
import '../engine/style.dart';
import '../engine/layout.dart' as engine;
import '../network/fetcher.dart';
import '../network/logger.dart';
import '../plugin/plugin.dart';
import '../plugin/plugin_pipeline.dart';
import '../plugin/built_in/cookie_manager.dart';
import '../plugin/built_in/identity_manager.dart';
import '../plugin/built_in/script_manager.dart';
import 'flutter_text_measurer.dart';
import 'identity_editor.dart';
import 'page_painter.dart';
import 'script_approval_dialog.dart';
import 'tab.dart' as tab_model;

class BrowserShell extends StatefulWidget {
  final PluginPipeline? pluginPipeline;
  final CookieManagerPlugin? cookieManager;
  final IdentityManagerPlugin? identityManager;
  final ScriptManagerPlugin? scriptManager;

  const BrowserShell({
    super.key,
    this.pluginPipeline,
    this.cookieManager,
    this.identityManager,
    this.scriptManager,
  });

  @override
  State<BrowserShell> createState() => _BrowserShellState();
}

class _BrowserShellState extends State<BrowserShell> {
  final List<tab_model.Tab> _tabs = [tab_model.Tab()];
  int _activeTabIndex = 0;
  final TextEditingController _addressController = TextEditingController();
  final FocusNode _addressFocus = FocusNode();
  final FlutterTextMeasurer _textMeasurer = FlutterTextMeasurer();

  // Phase 2: Find on page.
  bool _showFindBar = false;
  final TextEditingController _findController = TextEditingController();
  final FocusNode _findFocus = FocusNode();

  // Phase 2: View source.
  bool _showSource = false;

  // Phase 2: Text selection.
  Offset? _selectionStart;
  Offset? _selectionEnd;
  String _selectedText = '';

  tab_model.Tab get _activeTab => _tabs[_activeTabIndex];

  @override
  void dispose() {
    _addressController.dispose();
    _addressFocus.dispose();
    _findController.dispose();
    _findFocus.dispose();
    super.dispose();
  }

  // ── Navigation ──────────────────────────────────────────────────

  @override
  void initState() {
    super.initState();
    PaneLogger.info('Pane browser started — log file: ${PaneLogger.logPath}');
  }

  Future<void> _navigate(String input) async {
    if (input.trim().isEmpty) return;

    String url = input.trim();
    if (!url.startsWith('http://') && !url.startsWith('https://')) {
      url = 'https://$url';
    }

    // Plugin hook: onNavigate — may modify or cancel.
    final pipeline = widget.pluginPipeline;
    if (pipeline != null) {
      final result = pipeline.runNavigate(url);
      if (result == null) return; // cancelled by plugin
      url = result;
    }

    _activeTab.navigateTo(url);
    _addressController.text = url;

    await _loadPage(url);
  }

  Future<void> _loadPage(String url) async {
    setState(() {
      _activeTab.isLoading = true;
      _activeTab.errorMessage = null;
      _showSource = false;
    });

    final pipeline = widget.pluginPipeline;

    // Sync cookie plugin state with active tab's toggle.
    widget.cookieManager?.cookiesEnabled = _activeTab.cookiesEnabled;

    // Sync script manager state with active tab's settings.
    final sm = widget.scriptManager;
    if (sm != null) {
      sm.currentMode = _activeTab.scriptMode;
      sm.currentTransmitMode = _activeTab.scriptTransmitMode;
      sm.blockSubsequentRequests = false; // Allow initial page load.
    }

    try {
      // 1. Build a FetchRequest and run onBeforeRequest hooks.
      FetchResponse response;
      if (pipeline != null) {
        var request = FetchRequest(
          url: url,
          headers: {
            'Accept-Language': 'en-US,en;q=0.9',
            'Accept':
                'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
          },
        );
        final modified = pipeline.runBeforeRequest(request);
        if (modified == null) {
          setState(() => _activeTab.isLoading = false);
          return; // request cancelled by plugin
        }
        response = await Fetcher.fetchWithHeaders(
          modified.url,
          modified.headers,
        );
      } else {
        response = await Fetcher.fetch(url);
      }

      if (!response.isOk) {
        throw Exception('HTTP ${response.statusCode}');
      }

      // 2. Run onAfterResponse hooks.
      String htmlBody = response.body;
      String resolvedUrl = response.url;
      if (pipeline != null) {
        var responseData = FetchResponseData(
          statusCode: response.statusCode,
          body: response.body,
          headers: response.headers,
          url: response.url,
          contentType: response.contentType,
        );
        responseData = pipeline.runAfterResponse(responseData);
        htmlBody = responseData.body;
        resolvedUrl = responseData.url;
      }

      _activeTab.url = resolvedUrl;
      _addressController.text = resolvedUrl;
      _activeTab.sourceHtml = htmlBody;

      // 3. Parse HTML → DOM.
      final document = HtmlParser.parse(htmlBody);

      // 4. Run onDomReady hooks (plugins can inject/remove DOM nodes).
      //    Script manager extracts <script> tags here.
      pipeline?.runDomReady(document);

      // 4b. Execute JavaScript if enabled.
      if (sm != null && sm.hasPendingScripts) {
        if (_activeTab.scriptMode == ScriptMode.runAll) {
          final result = sm.executeAll(document);
          _activeTab.scriptLog = result.log.toList();
          if (result.pendingNavigation != null) {
            // Script wants to redirect — schedule it after rendering.
            Future.microtask(() => _navigate(result.pendingNavigation!));
          }
        } else if (_activeTab.scriptMode == ScriptMode.askEach) {
          // Show approval dialog (pauses pipeline).
          if (mounted) {
            await showScriptApprovalDialog(context, sm.pendingScripts);
            final result = sm.executeApproved(document);
            _activeTab.scriptLog = result.log.toList();
            if (result.pendingNavigation != null) {
              Future.microtask(() => _navigate(result.pendingNavigation!));
            }
          }
        }
      }

      // 5. Extract and parse CSS.
      final stylesheets = <css.Stylesheet>[];

      final internalCss = document.internalCSS;
      if (internalCss.isNotEmpty) {
        stylesheets.add(css.CssParser.parse(internalCss));
      }

      for (final href in document.externalStylesheetUrls) {
        try {
          final cssUrl = Fetcher.resolveUrl(resolvedUrl, href);
          final cssResponse = await Fetcher.fetch(cssUrl);
          if (cssResponse.isOk) {
            stylesheets.add(css.CssParser.parse(cssResponse.body));
          }
        } catch (e) {
          PaneLogger.warn('loadPage($url)', 'Failed to fetch stylesheet $href: $e');
        }
      }

      // 6. Compute styles.
      final body = document.body ?? document.documentElement ?? document;
      var styledTree = computeStyles(body, stylesheets);

      // 7. Run onStylesComputed hooks.
      if (pipeline != null) {
        styledTree = pipeline.runStylesComputed(styledTree, stylesheets);
      }

      // 8. Layout.
      final viewportWidth = _viewportWidth;
      var layoutRoot = engine.layoutTree(styledTree, viewportWidth, _textMeasurer);

      // 9. Run onLayoutComplete hooks.
      if (pipeline != null) {
        layoutRoot = pipeline.runLayoutComplete(layoutRoot);
      }

      // 10. Update tab state.
      setState(() {
        _activeTab.title = document.title.isNotEmpty
            ? document.title
            : _activeTab.url;
        _activeTab.layoutRoot = layoutRoot;
        _activeTab.pageHeight = _computePageHeight(layoutRoot);
        _activeTab.isLoading = false;
      });

      // 11. Fetch images in background.
      _fetchImages(layoutRoot, resolvedUrl);

      // 12. Activate transmission blocking if enabled.
      if (sm != null &&
          _activeTab.scriptTransmitMode == TransmitMode.blocked &&
          _activeTab.scriptMode != ScriptMode.off) {
        sm.blockSubsequentRequests = true;
      }
    } catch (e, stack) {
      PaneLogger.error('loadPage($url)', e, stack);
      setState(() {
        _activeTab.isLoading = false;
        _activeTab.errorMessage = e.toString();
        _activeTab.layoutRoot = null;
      });
    }
  }

  Future<void> _fetchImages(engine.LayoutBox root, String baseUrl) async {
    final imageBoxes = root.allBoxes.where((b) => b.imageUrl != null && b.imageUrl!.isNotEmpty).toList();
    for (final box in imageBoxes) {
      final src = box.imageUrl!;
      final resolvedUrl = Fetcher.resolveUrl(baseUrl, src);
      box.imageUrl = resolvedUrl;

      if (_activeTab.imageCache.containsKey(resolvedUrl)) continue;

      try {
        final bytes = await Fetcher.fetchBytes(resolvedUrl);
        if (bytes != null) {
          final codec = await ui.instantiateImageCodec(bytes);
          final frame = await codec.getNextFrame();
          if (mounted) {
            setState(() {
              _activeTab.imageCache[resolvedUrl] = frame.image;
              if (box.imageWidth == 300 && box.imageHeight == 150) {
                final imgW = frame.image.width.toDouble();
                final imgH = frame.image.height.toDouble();
                final maxW = box.content.width > 0 ? box.content.width : _viewportWidth;
                if (imgW > maxW) {
                  box.content.width = maxW;
                  box.content.height = imgH * (maxW / imgW);
                } else {
                  box.content.width = imgW;
                  box.content.height = imgH;
                }
              }
            });
          }
        }
      } catch (e) {
        PaneLogger.warn('fetchImages', 'Failed to load image $resolvedUrl: $e');
      }
    }
  }

  double get _viewportWidth {
    final mq = MediaQuery.of(context);
    return mq.size.width - 16;
  }

  double _computePageHeight(engine.LayoutBox root) {
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
    // Plugin hook: onLinkClick — may modify or cancel.
    final pipeline = widget.pluginPipeline;
    if (pipeline != null) {
      final result = pipeline.runLinkClick(href, _activeTab.url);
      if (result == null) return; // cancelled by plugin
      href = result;
    }

    final resolved = Fetcher.resolveUrl(_activeTab.url, href);
    _navigate(resolved);
  }

  // ── Tab management ──────────────────────────────────────────────

  void _addTab() {
    setState(() {
      _tabs.add(tab_model.Tab());
      _activeTabIndex = _tabs.length - 1;
      _addressController.text = '';
    });
    _addressFocus.requestFocus();
  }

  void _closeTab(int index) {
    if (_tabs.length <= 1) return;
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
      final viewportHeight = MediaQuery.of(context).size.height - 120;
      _activeTab.scrollOffset = (_activeTab.scrollOffset + delta).clamp(
        0.0,
        (_activeTab.pageHeight - viewportHeight).clamp(0.0, double.infinity),
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

  String? _hitTestLink(engine.LayoutBox box, double x, double y) {
    for (final child in box.children.reversed) {
      final result = _hitTestLink(child, x, y);
      if (result != null) return result;
    }

    if (box.linkHref != null && box.linkHref!.isNotEmpty) {
      final r = box.content;
      if (x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height) {
        return box.linkHref;
      }
    }

    return null;
  }

  // ── Text selection ──────────────────────────────────────────────

  void _onPanStart(DragStartDetails details) {
    setState(() {
      _selectionStart = Offset(
        details.localPosition.dx,
        details.localPosition.dy + _activeTab.scrollOffset,
      );
      _selectionEnd = _selectionStart;
      _selectedText = '';
    });
  }

  void _onPanUpdate(DragUpdateDetails details) {
    setState(() {
      _selectionEnd = Offset(
        details.localPosition.dx,
        details.localPosition.dy + _activeTab.scrollOffset,
      );
    });
  }

  void _onPanEnd(DragEndDetails details) {
    if (_activeTab.layoutRoot == null || _selectionStart == null || _selectionEnd == null) return;
    final text = _extractSelectedText(
      _activeTab.layoutRoot!,
      _selectionStart!,
      _selectionEnd!,
    );
    setState(() {
      _selectedText = text;
    });
  }

  String _extractSelectedText(engine.LayoutBox root, Offset start, Offset end) {
    final top = start.dy < end.dy ? start : end;
    final bottom = start.dy < end.dy ? end : start;

    final buf = StringBuffer();
    for (final box in root.allBoxes) {
      if (box.text == null || box.text!.isEmpty) continue;
      final r = box.content;
      final boxBottom = r.y + r.height;
      if (boxBottom >= top.dy && r.y <= bottom.dy) {
        buf.write(box.text);
      }
    }
    return buf.toString().trim();
  }

  void _copySelection() {
    if (_selectedText.isNotEmpty) {
      Clipboard.setData(ClipboardData(text: _selectedText));
    }
  }

  // ── Find on page ───────────────────────────────────────────────

  void _toggleFind() {
    setState(() {
      _showFindBar = !_showFindBar;
      if (_showFindBar) {
        _findFocus.requestFocus();
      } else {
        _activeTab.searchQuery = '';
        _activeTab.searchRects = [];
        _activeTab.searchIndex = -1;
      }
    });
  }

  void _performSearch(String query) {
    if (query.isEmpty || _activeTab.layoutRoot == null) {
      setState(() {
        _activeTab.searchQuery = '';
        _activeTab.searchRects = [];
        _activeTab.searchIndex = -1;
      });
      return;
    }

    final rects = <engine.Rect>[];
    final lowerQuery = query.toLowerCase();

    for (final box in _activeTab.layoutRoot!.allBoxes) {
      if (box.text == null || box.text!.isEmpty) continue;
      final text = box.text!.toLowerCase();
      int idx = 0;
      while ((idx = text.indexOf(lowerQuery, idx)) != -1) {
        final r = box.content;
        final charWidth = r.width / (box.text!.length.clamp(1, 9999));
        rects.add(engine.Rect(
          r.x + idx * charWidth,
          r.y,
          lowerQuery.length * charWidth,
          r.height,
        ));
        idx += lowerQuery.length;
      }
    }

    setState(() {
      _activeTab.searchQuery = query;
      _activeTab.searchRects = rects;
      _activeTab.searchIndex = rects.isNotEmpty ? 0 : -1;
      if (rects.isNotEmpty) {
        _activeTab.scrollOffset = (rects[0].y - 100).clamp(0.0, double.infinity);
      }
    });
  }

  void _findNext() {
    if (_activeTab.searchRects.isEmpty) return;
    setState(() {
      _activeTab.searchIndex =
          (_activeTab.searchIndex + 1) % _activeTab.searchRects.length;
      final r = _activeTab.searchRects[_activeTab.searchIndex];
      _activeTab.scrollOffset = (r.y - 100).clamp(0.0, double.infinity);
    });
  }

  void _findPrevious() {
    if (_activeTab.searchRects.isEmpty) return;
    setState(() {
      _activeTab.searchIndex =
          (_activeTab.searchIndex - 1 + _activeTab.searchRects.length) %
              _activeTab.searchRects.length;
      final r = _activeTab.searchRects[_activeTab.searchIndex];
      _activeTab.scrollOffset = (r.y - 100).clamp(0.0, double.infinity);
    });
  }

  // ── View Source ────────────────────────────────────────────────

  void _toggleViewSource() {
    setState(() {
      _showSource = !_showSource;
    });
  }

  // ── Keyboard shortcuts ───────────────────────────────────────────

  KeyEventResult _handleKeyEvent(FocusNode node, KeyEvent event) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;

    final ctrl = HardwareKeyboard.instance.isControlPressed;

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyL) {
      _addressFocus.requestFocus();
      _addressController.selection = TextSelection(
        baseOffset: 0,
        extentOffset: _addressController.text.length,
      );
      return KeyEventResult.handled;
    }

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyT) {
      _addTab();
      return KeyEventResult.handled;
    }

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyW) {
      _closeTab(_activeTabIndex);
      return KeyEventResult.handled;
    }

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyF) {
      _toggleFind();
      return KeyEventResult.handled;
    }

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyU) {
      _toggleViewSource();
      return KeyEventResult.handled;
    }

    if (ctrl && event.logicalKey == LogicalKeyboardKey.keyC) {
      _copySelection();
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.escape) {
      if (_showFindBar) {
        _toggleFind();
        return KeyEventResult.handled;
      }
      if (_showSource) {
        setState(() => _showSource = false);
        return KeyEventResult.handled;
      }
    }

    if (event.logicalKey == LogicalKeyboardKey.f5) {
      _reload();
      return KeyEventResult.handled;
    }

    if (HardwareKeyboard.instance.isAltPressed &&
        event.logicalKey == LogicalKeyboardKey.arrowLeft) {
      _goBack();
      return KeyEventResult.handled;
    }

    if (HardwareKeyboard.instance.isAltPressed &&
        event.logicalKey == LogicalKeyboardKey.arrowRight) {
      _goForward();
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.arrowDown && !_addressFocus.hasFocus && !_findFocus.hasFocus) {
      _onScroll(40);
      return KeyEventResult.handled;
    }
    if (event.logicalKey == LogicalKeyboardKey.arrowUp && !_addressFocus.hasFocus && !_findFocus.hasFocus) {
      _onScroll(-40);
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.pageDown ||
        (event.logicalKey == LogicalKeyboardKey.space && !_addressFocus.hasFocus && !_findFocus.hasFocus)) {
      _onScroll(300);
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.pageUp) {
      _onScroll(-300);
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.home && !_addressFocus.hasFocus && !_findFocus.hasFocus) {
      setState(() => _activeTab.scrollOffset = 0);
      return KeyEventResult.handled;
    }

    if (event.logicalKey == LogicalKeyboardKey.end && !_addressFocus.hasFocus && !_findFocus.hasFocus) {
      final viewportHeight = MediaQuery.of(context).size.height - 120;
      setState(() {
        _activeTab.scrollOffset =
            (_activeTab.pageHeight - viewportHeight).clamp(0.0, double.infinity);
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
            if (_showFindBar) _buildFindBar(),
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
          IconButton(
            icon: const Icon(Icons.arrow_back, size: 20),
            onPressed: _activeTab.canGoBack ? _goBack : null,
            tooltip: 'Back',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
          IconButton(
            icon: const Icon(Icons.arrow_forward, size: 20),
            onPressed: _activeTab.canGoForward ? _goForward : null,
            tooltip: 'Forward',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
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
          const SizedBox(width: 4),
          IconButton(
            icon: const Icon(Icons.code, size: 20),
            onPressed: _activeTab.sourceHtml != null ? _toggleViewSource : null,
            tooltip: 'View Source (Ctrl+U)',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 36),
          ),
          if (widget.scriptManager != null) ...[
            const SizedBox(width: 2),
            _buildScriptModeButton(),
          ],
          if (widget.identityManager != null) ...[
            const SizedBox(width: 2),
            _buildIdentityButton(),
          ],
          if (widget.cookieManager != null) ...[
            const SizedBox(width: 2),
            _buildCookieToggle(),
          ],
        ],
      ),
    );
  }

  Widget _buildScriptModeButton() {
    final mode = _activeTab.scriptMode;
    final transmit = _activeTab.scriptTransmitMode;
    final isOn = mode != ScriptMode.off;
    final sm = widget.scriptManager!;
    final scriptCount = sm.pendingScripts.length;

    Color bgColor;
    Color fgColor;
    Color borderColor;
    switch (mode) {
      case ScriptMode.off:
        bgColor = const Color(0xFFF5F5F5);
        fgColor = Colors.grey.shade600;
        borderColor = Colors.grey.shade400;
      case ScriptMode.runAll:
        bgColor = const Color(0xFFE8F5E9);
        fgColor = const Color(0xFF2E7D32);
        borderColor = const Color(0xFF66BB6A);
      case ScriptMode.askEach:
        bgColor = const Color(0xFFFFF3E0);
        fgColor = const Color(0xFFE65100);
        borderColor = const Color(0xFFFFB74D);
    }

    return PopupMenuButton<String>(
      offset: const Offset(0, 34),
      tooltip: 'JavaScript execution mode',
      onSelected: (value) {
        setState(() {
          switch (value) {
            case 'off':
              _activeTab.scriptMode = ScriptMode.off;
            case 'all':
              _activeTab.scriptMode = ScriptMode.runAll;
            case 'ask':
              _activeTab.scriptMode = ScriptMode.askEach;
            case 'toggle_transmit':
              _activeTab.scriptTransmitMode =
                  transmit == TransmitMode.normal
                      ? TransmitMode.blocked
                      : TransmitMode.normal;
          }
        });
      },
      itemBuilder: (context) => [
        PopupMenuItem(
          enabled: false,
          height: 28,
          child: Text('JavaScript Mode',
              style: TextStyle(
                  fontSize: 11,
                  fontWeight: FontWeight.w600,
                  color: Colors.grey.shade600)),
        ),
        _modeItem('off', 'Don\'t Run JavaScript', ScriptMode.off, mode),
        _modeItem('all', 'Run All JavaScript', ScriptMode.runAll, mode),
        _modeItem('ask', 'Ask for Each Script', ScriptMode.askEach, mode),
        if (isOn) ...[
          const PopupMenuDivider(),
          PopupMenuItem(
            enabled: false,
            height: 28,
            child: Text('Data Transmission',
                style: TextStyle(
                    fontSize: 11,
                    fontWeight: FontWeight.w600,
                    color: Colors.grey.shade600)),
          ),
          PopupMenuItem(
            value: 'toggle_transmit',
            height: 36,
            child: Row(
              children: [
                Icon(
                  transmit == TransmitMode.normal
                      ? Icons.cloud_upload_outlined
                      : Icons.cloud_off_outlined,
                  size: 16,
                  color: transmit == TransmitMode.blocked
                      ? Colors.red.shade400
                      : Colors.grey.shade700,
                ),
                const SizedBox(width: 8),
                Text(
                  transmit == TransmitMode.normal
                      ? 'Allow outbound data'
                      : 'Block outbound data',
                  style: const TextStyle(fontSize: 13),
                ),
                const Spacer(),
                if (transmit == TransmitMode.blocked)
                  Icon(Icons.check, size: 16, color: Colors.red.shade400),
              ],
            ),
          ),
        ],
        if (scriptCount > 0) ...[
          const PopupMenuDivider(),
          PopupMenuItem(
            enabled: false,
            height: 28,
            child: Text(
              '$scriptCount script${scriptCount == 1 ? '' : 's'} detected',
              style: TextStyle(fontSize: 11, color: Colors.grey.shade500),
            ),
          ),
        ],
      ],
      child: Container(
        height: 26,
        padding: const EdgeInsets.symmetric(horizontal: 6),
        decoration: BoxDecoration(
          color: bgColor,
          borderRadius: BorderRadius.circular(4),
          border: Border.all(color: borderColor, width: 0.5),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              'JS',
              style: TextStyle(
                fontSize: 11,
                fontWeight: FontWeight.w700,
                color: fgColor,
              ),
            ),
            const SizedBox(width: 2),
            Text(
              mode.label,
              style: TextStyle(
                fontSize: 10,
                fontWeight: FontWeight.w500,
                color: fgColor,
              ),
            ),
            if (isOn && transmit == TransmitMode.blocked) ...[
              const SizedBox(width: 3),
              Icon(Icons.cloud_off, size: 12, color: Colors.red.shade400),
            ],
          ],
        ),
      ),
    );
  }

  PopupMenuItem<String> _modeItem(
    String value,
    String label,
    ScriptMode itemMode,
    ScriptMode current,
  ) {
    return PopupMenuItem(
      value: value,
      height: 36,
      child: Row(
        children: [
          Icon(
            itemMode == current
                ? Icons.radio_button_checked
                : Icons.radio_button_unchecked,
            size: 16,
            color: itemMode == current
                ? Theme.of(context).primaryColor
                : Colors.grey.shade400,
          ),
          const SizedBox(width: 8),
          Text(label, style: const TextStyle(fontSize: 13)),
        ],
      ),
    );
  }

  Widget _buildIdentityButton() {
    final mgr = widget.identityManager!;
    final isActive = mgr.activeProfile.fields.isNotEmpty;
    final profileName = mgr.activeProfile.name;

    return GestureDetector(
      onTap: () {
        showIdentityEditor(context, mgr, onChanged: () {
          setState(() {});
        });
      },
      child: Tooltip(
        message: isActive
            ? 'Identity: $profileName — click to edit'
            : 'Identity: None — click to configure',
        child: Container(
          height: 26,
          padding: const EdgeInsets.symmetric(horizontal: 6),
          decoration: BoxDecoration(
            color: isActive
                ? const Color(0xFFE3F2FD)
                : const Color(0xFFF5F5F5),
            borderRadius: BorderRadius.circular(4),
            border: Border.all(
              color: isActive
                  ? const Color(0xFF42A5F5)
                  : Colors.grey.shade400,
              width: 0.5,
            ),
          ),
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(
                Icons.fingerprint,
                size: 16,
                color: isActive
                    ? const Color(0xFF1565C0)
                    : Colors.grey.shade600,
              ),
              const SizedBox(width: 3),
              Text(
                isActive ? 'ID' : 'ID',
                style: TextStyle(
                  fontSize: 11,
                  fontWeight: FontWeight.w600,
                  color: isActive
                      ? const Color(0xFF1565C0)
                      : Colors.grey.shade600,
                ),
              ),
              if (isActive) ...[
                const SizedBox(width: 2),
                Container(
                  width: 6,
                  height: 6,
                  decoration: const BoxDecoration(
                    color: Color(0xFF42A5F5),
                    shape: BoxShape.circle,
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildCookieToggle() {
    final cm = widget.cookieManager!;
    final enabled = _activeTab.cookiesEnabled;
    final count = cm.cookieCount;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        GestureDetector(
          onTap: () {
            setState(() {
              _activeTab.cookiesEnabled = !_activeTab.cookiesEnabled;
              cm.cookiesEnabled = _activeTab.cookiesEnabled;
            });
          },
          child: Tooltip(
            message: enabled
                ? 'Cookies: ON ($count stored) — click to disable'
                : 'Cookies: OFF — click to enable',
            child: Container(
              height: 26,
              padding: const EdgeInsets.symmetric(horizontal: 6),
              decoration: BoxDecoration(
                color: enabled ? const Color(0xFFE8F5E9) : const Color(0xFFF5F5F5),
                borderRadius: BorderRadius.circular(4),
                border: Border.all(
                  color: enabled ? const Color(0xFF66BB6A) : Colors.grey.shade400,
                  width: 0.5,
                ),
              ),
              child: Row(
                children: [
                  Icon(
                    Icons.cookie_outlined,
                    size: 16,
                    color: enabled ? const Color(0xFF2E7D32) : Colors.grey.shade600,
                  ),
                  const SizedBox(width: 3),
                  Text(
                    enabled ? 'ON' : 'OFF',
                    style: TextStyle(
                      fontSize: 11,
                      fontWeight: FontWeight.w600,
                      color: enabled ? const Color(0xFF2E7D32) : Colors.grey.shade600,
                    ),
                  ),
                  if (enabled && count > 0) ...[
                    const SizedBox(width: 3),
                    Text(
                      '($count)',
                      style: TextStyle(
                        fontSize: 10,
                        color: Colors.grey.shade600,
                      ),
                    ),
                  ],
                ],
              ),
            ),
          ),
        ),
        const SizedBox(width: 2),
        IconButton(
          icon: Icon(
            Icons.delete_outline,
            size: 18,
            color: count > 0 ? Colors.red.shade400 : Colors.grey.shade400,
          ),
          onPressed: count > 0
              ? () {
                  setState(() {
                    cm.purgeAll();
                  });
                }
              : null,
          tooltip: count > 0 ? 'Purge all cookies ($count)' : 'No cookies to purge',
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(maxWidth: 30),
        ),
      ],
    );
  }

  Widget _buildFindBar() {
    final count = _activeTab.searchRects.length;
    final idx = _activeTab.searchIndex;
    return Container(
      height: 40,
      padding: const EdgeInsets.symmetric(horizontal: 8),
      decoration: BoxDecoration(
        color: const Color(0xFFFFF8E1),
        border: Border(bottom: BorderSide(color: Colors.grey.shade300)),
      ),
      child: Row(
        children: [
          const Icon(Icons.search, size: 18, color: Colors.grey),
          const SizedBox(width: 8),
          SizedBox(
            width: 200,
            child: TextField(
              controller: _findController,
              focusNode: _findFocus,
              style: const TextStyle(fontSize: 13),
              decoration: const InputDecoration(
                hintText: 'Find on page...',
                hintStyle: TextStyle(fontSize: 13, color: Colors.grey),
                border: InputBorder.none,
                isDense: true,
                contentPadding: EdgeInsets.symmetric(vertical: 8),
              ),
              onChanged: _performSearch,
              onSubmitted: (_) => _findNext(),
            ),
          ),
          if (count > 0) ...[
            const SizedBox(width: 8),
            Text(
              '${idx + 1}/$count',
              style: TextStyle(fontSize: 12, color: Colors.grey.shade700),
            ),
          ],
          IconButton(
            icon: const Icon(Icons.keyboard_arrow_up, size: 18),
            onPressed: _findPrevious,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 30),
            tooltip: 'Previous',
          ),
          IconButton(
            icon: const Icon(Icons.keyboard_arrow_down, size: 18),
            onPressed: _findNext,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 30),
            tooltip: 'Next',
          ),
          IconButton(
            icon: const Icon(Icons.close, size: 16),
            onPressed: _toggleFind,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(maxWidth: 30),
            tooltip: 'Close',
          ),
        ],
      ),
    );
  }

  Widget _buildContent() {
    if (_activeTab.errorMessage != null) {
      return _buildErrorPage();
    }

    if (_showSource && _activeTab.sourceHtml != null) {
      return _buildSourceView();
    }

    if (_activeTab.layoutRoot == null && !_activeTab.isLoading) {
      return _buildStartPage();
    }

    if (_activeTab.isLoading && _activeTab.layoutRoot == null) {
      return const Center(child: CircularProgressIndicator());
    }

    return LayoutBuilder(
      builder: (context, constraints) {
        final viewportHeight = constraints.maxHeight;
        final pageHeight = _activeTab.pageHeight;
        final scrollFraction = pageHeight > viewportHeight
            ? _activeTab.scrollOffset / (pageHeight - viewportHeight)
            : 0.0;
        final double thumbHeight = pageHeight > 0
            ? (viewportHeight / pageHeight * viewportHeight).clamp(30.0, viewportHeight).toDouble()
            : viewportHeight;

        return Stack(
          children: [
            Listener(
              onPointerSignal: (event) {
                if (event is PointerScrollEvent) {
                  _onScroll(event.scrollDelta.dy);
                }
              },
              child: GestureDetector(
                onTapDown: (details) => _onTapPage(details.localPosition),
                onPanStart: _onPanStart,
                onPanUpdate: _onPanUpdate,
                onPanEnd: _onPanEnd,
                child: ClipRect(
                  child: Stack(
                    children: [
                      CustomPaint(
                        painter: PagePainter(
                          rootBox: _activeTab.layoutRoot,
                          scrollOffset: _activeTab.scrollOffset,
                          imageCache: _activeTab.imageCache,
                          searchQuery: _activeTab.searchQuery,
                          currentSearchIndex: _activeTab.searchIndex,
                          searchRects: _activeTab.searchRects,
                          pluginPipeline: widget.pluginPipeline,
                        ),
                        size: Size.infinite,
                      ),
                      if (_selectionStart != null && _selectionEnd != null)
                        Positioned.fill(
                          child: IgnorePointer(
                            child: CustomPaint(
                              painter: _SelectionPainter(
                                start: _selectionStart!,
                                end: _selectionEnd!,
                                scrollOffset: _activeTab.scrollOffset,
                              ),
                            ),
                          ),
                        ),
                    ],
                  ),
                ),
              ),
            ),
            // Scrollbar.
            if (pageHeight > viewportHeight)
              Positioned(
                right: 0,
                top: 0,
                bottom: 0,
                width: 8,
                child: GestureDetector(
                  onVerticalDragUpdate: (details) {
                    final fraction = details.localPosition.dy / viewportHeight;
                    setState(() {
                      _activeTab.scrollOffset = (fraction * (pageHeight - viewportHeight))
                          .clamp(0.0, (pageHeight - viewportHeight).clamp(0.0, double.infinity));
                    });
                  },
                  child: Container(
                    color: Colors.grey.shade200.withValues(alpha: 0.5),
                    child: Align(
                      alignment: Alignment.topCenter,
                      child: Container(
                        margin: EdgeInsets.only(top: scrollFraction * (viewportHeight - thumbHeight)),
                        width: 8,
                        height: thumbHeight,
                        decoration: BoxDecoration(
                          color: Colors.grey.shade500.withValues(alpha: 0.6),
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                    ),
                  ),
                ),
              ),
            if (_selectedText.isNotEmpty)
              Positioned(
                bottom: 4,
                right: 16,
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: Colors.black87,
                    borderRadius: BorderRadius.circular(4),
                  ),
                  child: Text(
                    'Ctrl+C to copy (${_selectedText.length} chars)',
                    style: const TextStyle(color: Colors.white, fontSize: 11),
                  ),
                ),
              ),
          ],
        );
      },
    );
  }

  Widget _buildSourceView() {
    return Container(
      color: const Color(0xFF1E1E1E),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
            color: const Color(0xFF2D2D2D),
            child: Row(
              children: [
                const Text(
                  'Page Source',
                  style: TextStyle(color: Colors.white70, fontSize: 13, fontWeight: FontWeight.w500),
                ),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.copy, size: 16, color: Colors.white70),
                  onPressed: () {
                    Clipboard.setData(ClipboardData(text: _activeTab.sourceHtml ?? ''));
                  },
                  tooltip: 'Copy source',
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(maxWidth: 30),
                ),
                IconButton(
                  icon: const Icon(Icons.close, size: 16, color: Colors.white70),
                  onPressed: _toggleViewSource,
                  tooltip: 'Close',
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(maxWidth: 30),
                ),
              ],
            ),
          ),
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(12),
              child: SelectableText(
                _activeTab.sourceHtml ?? '',
                style: const TextStyle(
                  color: Color(0xFFD4D4D4),
                  fontSize: 12,
                  fontFamily: 'monospace',
                  height: 1.5,
                ),
              ),
            ),
          ),
        ],
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
            const SizedBox(height: 12),
            Text(
              'Log: ${PaneLogger.logPath}',
              style: TextStyle(fontSize: 11, color: Colors.grey.shade400),
            ),
          ],
        ),
      ),
    );
  }
}

/// Paints a translucent selection rectangle.
class _SelectionPainter extends CustomPainter {
  final Offset start;
  final Offset end;
  final double scrollOffset;

  _SelectionPainter({required this.start, required this.end, required this.scrollOffset});

  @override
  void paint(Canvas canvas, Size size) {
    final s = Offset(start.dx, start.dy - scrollOffset);
    final e = Offset(end.dx, end.dy - scrollOffset);

    canvas.drawRect(
      Rect.fromPoints(s, e),
      Paint()..color = Colors.blue.withValues(alpha: 0.2),
    );
  }

  @override
  bool shouldRepaint(covariant _SelectionPainter oldDelegate) {
    return start != oldDelegate.start || end != oldDelegate.end || scrollOffset != oldDelegate.scrollOffset;
  }
}
