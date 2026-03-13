/// Privacy Shield — built-in plugin that strips tracking parameters
/// and blocks known tracker domains.

import '../plugin.dart';
import '../plugin_context.dart';

class PrivacyShieldPlugin extends Plugin {
  final PluginContext _context;
  PrivacyShieldPlugin(this._context);

  @override
  String get name => 'privacy_shield';

  /// Known tracking query-parameter names.
  static const _trackingParams = {
    'utm_source', 'utm_medium', 'utm_campaign', 'utm_term', 'utm_content',
    'fbclid', 'gclid', 'msclkid', 'twclid', 'mc_eid', 'yclid',
    'ref', '_ga', 'dclid', 'igshid',
  };

  /// Known tracker / analytics domains to block entirely.
  static const _blockedDomains = {
    'google-analytics.com',
    'googletagmanager.com',
    'facebook.net',
    'doubleclick.net',
    'hotjar.com',
    'segment.io',
    'mixpanel.com',
  };

  @override
  FetchRequest? onBeforeRequest(FetchRequest request) {
    final uri = Uri.tryParse(request.url);
    if (uri == null) return request;

    // Block known tracker domains.
    if (_blockedDomains.any((d) => uri.host.endsWith(d))) {
      return null; // cancel the request
    }

    // Strip tracking query parameters.
    final cleanParams = Map<String, String>.from(uri.queryParameters)
      ..removeWhere((key, _) => _trackingParams.contains(key.toLowerCase()));

    request.url = uri
        .replace(
            queryParameters: cleanParams.isEmpty ? null : cleanParams)
        .toString();

    // Strip referer header.
    request.headers.remove('Referer');
    request.headers.remove('referer');

    return request;
  }

  @override
  String? onNavigate(String url) {
    // Strip tracking params from navigation URLs too.
    final uri = Uri.tryParse(url);
    if (uri == null) return url;

    final clean = Map<String, String>.from(uri.queryParameters)
      ..removeWhere((key, _) => _trackingParams.contains(key.toLowerCase()));

    return uri
        .replace(queryParameters: clean.isEmpty ? null : clean)
        .toString();
  }
}
