/// Cookie Manager — built-in plugin that gives per-tab cookie control.
///
/// Parses Set-Cookie response headers, stores cookies in a file-based
/// jar, and injects Cookie headers into requests when enabled.
/// The browser shell controls the [cookiesEnabled] flag per-tab.

import 'dart:io';
import 'dart:convert';

import '../plugin.dart';
import '../../network/logger.dart';

class CookieManagerPlugin extends Plugin {
  @override
  String get name => 'cookie_manager';

  /// Whether cookies are currently enabled (set by the browser shell
  /// based on the active tab's toggle before each request cycle).
  bool cookiesEnabled = false;

  /// In-memory cookie jar: domain -> {name: value}.
  final Map<String, Map<String, String>> _jar = {};

  File? _file;

  /// Resolve the cookie storage file path.
  File get _cookieFile {
    if (_file != null) return _file!;
    final home = Platform.environment['HOME'] ??
        Platform.environment['USERPROFILE'] ??
        '.';
    _file = File('$home/.pane_cookies.json');
    return _file!;
  }

  @override
  Future<void> onEnable() async {
    _loadFromDisk();
  }

  // ── Network hooks ─────────────────────────────────────────

  @override
  FetchRequest? onBeforeRequest(FetchRequest request) {
    if (!cookiesEnabled) return request;

    final host = Uri.tryParse(request.url)?.host ?? '';
    if (host.isEmpty) return request;

    // Collect cookies that match the request domain.
    final matching = <String, String>{};
    for (final entry in _jar.entries) {
      if (_domainMatches(host, entry.key)) {
        matching.addAll(entry.value);
      }
    }

    if (matching.isNotEmpty) {
      request.headers['Cookie'] =
          matching.entries.map((e) => '${e.key}=${e.value}').join('; ');
    }

    return request;
  }

  @override
  FetchResponseData onAfterResponse(FetchResponseData response) {
    if (!cookiesEnabled) return response;

    // Parse Set-Cookie headers from the response.
    final setCookie = response.headers['set-cookie'];
    if (setCookie == null || setCookie.isEmpty) return response;

    final host = Uri.tryParse(response.url)?.host ?? '';
    if (host.isEmpty) return response;

    // set-cookie values may be joined with ', ' by the header flattener.
    // Each cookie starts a new name=value pair. Split carefully.
    for (final raw in _splitSetCookieHeader(setCookie)) {
      _parseCookie(raw, host);
    }

    _saveToDisk();

    return response;
  }

  // ── Public API for the browser shell ──────────────────────

  /// Number of cookies currently stored.
  int get cookieCount {
    int count = 0;
    for (final domain in _jar.values) {
      count += domain.length;
    }
    return count;
  }

  /// Purge all stored cookies from memory and disk.
  void purgeAll() {
    _jar.clear();
    try {
      if (_cookieFile.existsSync()) {
        _cookieFile.deleteSync();
      }
    } catch (e) {
      PaneLogger.warn('cookie_manager', 'Failed to delete cookie file: $e');
    }
  }

  // ── Internals ─────────────────────────────────────────────

  /// Parse a single Set-Cookie value and store it.
  void _parseCookie(String raw, String requestHost) {
    final trimmed = raw.trim();
    if (trimmed.isEmpty) return;

    // The first part before ';' is the name=value pair.
    final parts = trimmed.split(';');
    final nameValue = parts[0].trim();

    final eqIdx = nameValue.indexOf('=');
    if (eqIdx <= 0) return;

    final cookieName = nameValue.substring(0, eqIdx).trim();
    final cookieValue = nameValue.substring(eqIdx + 1).trim();

    // Look for Domain attribute — fall back to request host.
    String domain = requestHost;
    for (int i = 1; i < parts.length; i++) {
      final attr = parts[i].trim().toLowerCase();
      if (attr.startsWith('domain=')) {
        domain = attr.substring(7).trim();
        if (domain.startsWith('.')) domain = domain.substring(1);
        break;
      }
    }

    _jar.putIfAbsent(domain, () => {});
    _jar[domain]![cookieName] = cookieValue;
  }

  /// Split a flattened set-cookie header back into individual cookies.
  /// Cookies can contain commas in values (e.g., date strings), so we
  /// look for patterns like `, <name>=` to split.
  List<String> _splitSetCookieHeader(String header) {
    final cookies = <String>[];
    // Simple approach: split on `, ` but only when the next token looks
    // like a new cookie (contains `=` before any `;`).
    final parts = header.split(', ');
    final buf = StringBuffer();

    for (int i = 0; i < parts.length; i++) {
      if (i == 0) {
        buf.write(parts[i]);
        continue;
      }

      // Check if this looks like a new cookie (has = before ;).
      final eqIdx = parts[i].indexOf('=');
      final scIdx = parts[i].indexOf(';');
      final looksLikeCookie = eqIdx > 0 && (scIdx < 0 || eqIdx < scIdx);

      if (looksLikeCookie) {
        cookies.add(buf.toString());
        buf.clear();
        buf.write(parts[i]);
      } else {
        buf.write(', ');
        buf.write(parts[i]);
      }
    }

    if (buf.isNotEmpty) {
      cookies.add(buf.toString());
    }

    return cookies;
  }

  /// Check if [requestHost] matches [cookieDomain].
  bool _domainMatches(String requestHost, String cookieDomain) {
    if (requestHost == cookieDomain) return true;
    if (requestHost.endsWith('.$cookieDomain')) return true;
    return false;
  }

  /// Load cookie jar from disk.
  void _loadFromDisk() {
    try {
      final file = _cookieFile;
      if (!file.existsSync()) return;
      final content = file.readAsStringSync();
      if (content.trim().isEmpty) return;
      final decoded = jsonDecode(content) as Map<String, dynamic>;
      _jar.clear();
      for (final entry in decoded.entries) {
        _jar[entry.key] = Map<String, String>.from(entry.value as Map);
      }
    } catch (e) {
      PaneLogger.warn('cookie_manager', 'Failed to load cookies: $e');
    }
  }

  /// Save cookie jar to disk.
  void _saveToDisk() {
    try {
      _cookieFile.writeAsStringSync(
        jsonEncode(_jar),
        flush: true,
      );
    } catch (e) {
      PaneLogger.warn('cookie_manager', 'Failed to save cookies: $e');
    }
  }
}
