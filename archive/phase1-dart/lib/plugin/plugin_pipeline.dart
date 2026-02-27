/// Plugin pipeline — dispatches hooks to enabled plugins in chain order.
///
/// Each hook iterates over enabled plugins sequentially. Plugins see each
/// other's modifications (the output of one feeds into the next). For
/// hooks that can cancel (onBeforeRequest, onNavigate, onLinkClick),
/// returning null from any plugin stops the chain.

import 'dart:ui';

import '../engine/dom.dart';
import '../engine/css.dart';
import '../engine/style.dart';
import '../engine/layout.dart';
import 'plugin.dart';
import 'plugin_context.dart';
import 'plugin_registry.dart';

class PluginPipeline {
  final PluginRegistry registry;
  final PluginContext context;

  PluginPipeline(this.registry, this.context);

  // ── Network hooks ─────────────────────────────────────────

  /// Run onBeforeRequest across all enabled plugins.
  /// Returns null if any plugin cancels the request.
  FetchRequest? runBeforeRequest(FetchRequest request) {
    // Apply context first (spoofed UA, cookies, custom headers).
    context.applyToRequest(request);

    FetchRequest? current = request;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onBeforeRequest(current!);
      if (current == null) return null;
    }
    return current;
  }

  FetchResponseData runAfterResponse(FetchResponseData response) {
    var current = response;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onAfterResponse(current);
    }
    return current;
  }

  // ── DOM hook ──────────────────────────────────────────────

  void runDomReady(Document document) {
    for (final plugin in registry.enabledPlugins) {
      plugin.onDomReady(document);
    }
  }

  // ── Style hook ────────────────────────────────────────────

  StyledNode runStylesComputed(StyledNode tree, List<Stylesheet> sheets) {
    var current = tree;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onStylesComputed(current, sheets);
    }
    return current;
  }

  // ── Layout hook ───────────────────────────────────────────

  LayoutBox runLayoutComplete(LayoutBox root) {
    var current = root;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onLayoutComplete(current);
    }
    return current;
  }

  // ── Paint hook ────────────────────────────────────────────

  void runPaint(Canvas canvas, Size size, double scrollOffset) {
    for (final plugin in registry.enabledPlugins) {
      plugin.onPaint(canvas, size, scrollOffset);
    }
  }

  // ── Navigation hooks ──────────────────────────────────────

  /// Run onNavigate. Returns null if any plugin cancels navigation.
  String? runNavigate(String url) {
    String? current = url;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onNavigate(current!);
      if (current == null) return null;
    }
    return current;
  }

  /// Run onLinkClick. Returns null if any plugin cancels the click.
  String? runLinkClick(String href, String baseUrl) {
    String? current = href;
    for (final plugin in registry.enabledPlugins) {
      current = plugin.onLinkClick(current!, baseUrl);
      if (current == null) return null;
    }
    return current;
  }
}
