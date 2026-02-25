/// Network fetcher — uses dart:io for HTTP/HTTPS.
///
/// Minimal request footprint: no cookies, no referrer, no tracking headers.
/// Returns the response body, status code, headers, and resolved URL.

import 'dart:io';
import 'dart:convert';

class FetchResponse {
  final int statusCode;
  final String body;
  final Map<String, String> headers;
  final String url;
  final String? contentType;

  FetchResponse({
    required this.statusCode,
    required this.body,
    required this.headers,
    required this.url,
    this.contentType,
  });

  bool get isOk => statusCode >= 200 && statusCode < 300;
  bool get isRedirect => statusCode >= 300 && statusCode < 400;
}

class Fetcher {
  static final HttpClient _client = HttpClient()
    ..userAgent = 'Pane/0.1'
    ..connectionTimeout = const Duration(seconds: 10);

  /// Fetch a URL. Follows redirects (up to 5). No cookies or referrer.
  static Future<FetchResponse> fetch(String url) async {
    var currentUrl = url;
    const maxRedirects = 5;

    for (int i = 0; i < maxRedirects; i++) {
      final uri = Uri.parse(currentUrl);

      final request = await _client.getUrl(uri);
      // Strip default headers — minimal footprint.
      request.headers.removeAll('cookie');
      request.headers.removeAll('referer');

      // Disable auto-redirect so we can handle it manually.
      request.followRedirects = false;

      final response = await request.close();
      final statusCode = response.statusCode;

      if (statusCode >= 300 && statusCode < 400) {
        final location = response.headers.value('location');
        if (location == null) break;
        // Resolve relative redirects.
        currentUrl = uri.resolve(location).toString();
        // Drain the response body.
        await response.drain();
        continue;
      }

      // Read the body.
      final contentType = response.headers.contentType;
      final charset = contentType?.charset ?? 'utf-8';

      String body;
      try {
        body = await response.transform(
          Encoding.getByName(charset) != null
              ? Encoding.getByName(charset)!.decoder
              : utf8.decoder,
        ).join();
      } catch (_) {
        body = await response.transform(utf8.decoder).join();
      }

      final headers = <String, String>{};
      response.headers.forEach((name, values) {
        headers[name] = values.join(', ');
      });

      return FetchResponse(
        statusCode: statusCode,
        body: body,
        headers: headers,
        url: currentUrl,
        contentType: contentType?.mimeType,
      );
    }

    return FetchResponse(
      statusCode: 0,
      body: 'Too many redirects',
      headers: {},
      url: currentUrl,
    );
  }

  /// Resolve a possibly-relative URL against a base URL.
  static String resolveUrl(String base, String href) {
    if (href.startsWith('http://') || href.startsWith('https://')) {
      return href;
    }
    try {
      return Uri.parse(base).resolve(href).toString();
    } catch (_) {
      return href;
    }
  }
}
