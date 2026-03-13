/// Plugin system — abstract base class and hook data objects.
///
/// Every Pane plugin extends [Plugin] and overrides only the hooks it needs.
/// All methods have default no-op implementations. The [PluginPipeline]
/// chains enabled plugins in registration order at each pipeline stage.

import 'dart:ui';

import '../engine/dom.dart';
import '../engine/css.dart';
import '../engine/style.dart';
import '../engine/layout.dart';

// ── Hook data objects ────────────────────────────────────────────────

/// Mutable request data that plugins can inspect and modify.
class FetchRequest {
  String url;
  String method;
  Map<String, String> headers;
  String? body;

  FetchRequest({
    required this.url,
    this.method = 'GET',
    Map<String, String>? headers,
    this.body,
  }) : headers = headers ?? {};
}

/// Mutable response data that plugins can inspect and modify.
class FetchResponseData {
  int statusCode;
  String body;
  Map<String, String> headers;
  String url;
  String? contentType;

  FetchResponseData({
    required this.statusCode,
    required this.body,
    required this.headers,
    required this.url,
    this.contentType,
  });
}

// ── Abstract plugin base ─────────────────────────────────────────────

/// Base class for all Pane plugins.
///
/// Override only the hooks you need — everything defaults to pass-through.
abstract class Plugin {
  /// Unique identifier (must match the manifest name).
  String get name;

  // ── Lifecycle ─────────────────────────────────────────────
  Future<void> onInstall() async {}
  Future<void> onEnable() async {}
  Future<void> onDisable() async {}
  Future<void> onUninstall() async {}

  // ── Network hooks ─────────────────────────────────────────
  /// Called before every HTTP request. Modify url, headers, etc.
  /// Return null to cancel the request entirely.
  FetchRequest? onBeforeRequest(FetchRequest request) => request;

  /// Called after receiving an HTTP response, before HTML parsing.
  /// Can modify the response body, headers, or status.
  FetchResponseData onAfterResponse(FetchResponseData response) => response;

  // ── DOM hook ──────────────────────────────────────────────
  /// Called after HTML is parsed into a Document. Can mutate the DOM tree
  /// (inject elements, remove tracking nodes, etc.).
  void onDomReady(Document document) {}

  // ── Style hook ────────────────────────────────────────────
  /// Called after styles are computed. Can modify the styled tree or
  /// inject additional stylesheets.
  StyledNode onStylesComputed(
    StyledNode styledTree,
    List<Stylesheet> stylesheets,
  ) =>
      styledTree;

  // ── Layout hook ───────────────────────────────────────────
  /// Called after layout is complete. Can modify layout boxes.
  LayoutBox onLayoutComplete(LayoutBox layoutRoot) => layoutRoot;

  // ── Paint hook ────────────────────────────────────────────
  /// Called at the end of painting. Draw overlays, debug outlines, etc.
  void onPaint(Canvas canvas, Size size, double scrollOffset) {}

  // ── Navigation hooks ──────────────────────────────────────
  /// Called when the user navigates to a URL. Return the (possibly
  /// modified) URL, or null to cancel navigation.
  String? onNavigate(String url) => url;

  /// Called when the user clicks a link. Return the (possibly modified)
  /// href, or null to cancel the click.
  String? onLinkClick(String href, String baseUrl) => href;
}
