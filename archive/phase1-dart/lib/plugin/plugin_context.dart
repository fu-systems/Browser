/// Plugin context — shared mutable state for site-interaction control.
///
/// Plugins read/write this context to control browser behaviour such as
/// spoofed User-Agent, screen dimensions, cookie policies, and custom
/// request headers. The pipeline applies context values to each outgoing
/// request automatically.

import 'plugin.dart';

class PluginContext {
  // ── Spoofed identity / environment ──
  String? userAgent;
  String? acceptLanguage;
  int? screenWidth;
  int? screenHeight;

  // ── Cookie control ──
  /// Per-domain cookie policy: `true` = allow, `false` = block.
  final Map<String, bool> cookiePolicies = {};

  /// Editable cookie jar: domain -> {name: value}.
  final Map<String, Map<String, String>> cookies = {};

  // ── Custom request headers ──
  /// Extra headers merged into every request.
  final Map<String, String> customHeaders = {};

  /// Apply context values to a [FetchRequest] before it is sent.
  void applyToRequest(FetchRequest request) {
    if (userAgent != null) {
      request.headers['User-Agent'] = userAgent!;
    }
    if (acceptLanguage != null) {
      request.headers['Accept-Language'] = acceptLanguage!;
    }

    // Merge custom headers.
    request.headers.addAll(customHeaders);

    // Cookie handling.
    final host = Uri.tryParse(request.url)?.host ?? '';
    if (cookiePolicies[host] == false) {
      // Domain is blocked — strip any cookie header.
      request.headers.remove('Cookie');
    } else if (cookies.containsKey(host)) {
      final jar = cookies[host]!;
      if (jar.isNotEmpty) {
        request.headers['Cookie'] =
            jar.entries.map((e) => '${e.key}=${e.value}').join('; ');
      }
    }
  }
}
