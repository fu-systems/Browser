/// Identity profile — data model for browser fingerprint spoofing.
///
/// Defines the full set of fields that websites can probe for
/// fingerprinting, organized into categories. Includes factory
/// constructors for realistic preset profiles.

// ── Field & category definitions ────────────────────────────────

/// One editable field in a profile.
class FieldDef {
  final String key;
  final String label;
  final String hint;
  const FieldDef(this.key, this.label, [this.hint = '']);
}

/// A group of related fields shown together in the editor.
class CategoryDef {
  final String name;
  final List<FieldDef> fields;
  const CategoryDef(this.name, this.fields);
}

/// Master list of every fingerprint field, organized by category.
const List<CategoryDef> profileFieldDefs = [
  // ── Browser & Engine ──
  CategoryDef('Browser & Engine', [
    FieldDef('userAgent', 'User-Agent', 'Full UA header string'),
    FieldDef('browserName', 'Browser Name', 'e.g. Edge, Safari, Firefox'),
    FieldDef('browserVersion', 'Browser Version', 'e.g. 121.0.2277.83'),
    FieldDef('engineName', 'Rendering Engine', 'e.g. Blink, WebKit, Gecko'),
    FieldDef('engineVersion', 'Engine Version', 'e.g. 537.36'),
    FieldDef('appName', 'navigator.appName', 'Always "Netscape"'),
    FieldDef('appVersion', 'navigator.appVersion', 'Partial UA after "5.0"'),
    FieldDef('product', 'navigator.product', 'Usually "Gecko"'),
    FieldDef('productSub', 'navigator.productSub', 'e.g. 20030107'),
    FieldDef('vendor', 'navigator.vendor', 'e.g. Google Inc.'),
    FieldDef('vendorSub', 'navigator.vendorSub', 'Usually empty'),
    FieldDef('buildID', 'navigator.buildID', 'Firefox only'),
  ]),

  // ── Operating System ──
  CategoryDef('Operating System', [
    FieldDef('osName', 'OS Name', 'e.g. Windows, macOS, Linux'),
    FieldDef('osVersion', 'OS Version', 'e.g. 10.0, 14.2'),
    FieldDef('platform', 'navigator.platform', 'e.g. Win32, MacIntel'),
    FieldDef('architecture', 'Architecture', 'e.g. x86_64, arm64'),
    FieldDef('bitness', 'Bitness', '64 or 32'),
    FieldDef('mobile', 'Is Mobile', 'true or false'),
    FieldDef('oscpu', 'navigator.oscpu', 'Firefox only'),
  ]),

  // ── Client Hints (HTTP headers) ──
  CategoryDef('Client Hints (HTTP)', [
    FieldDef('secChUa', 'Sec-CH-UA', 'Brand list with versions'),
    FieldDef('secChUaPlatform', 'Sec-CH-UA-Platform', 'Quoted platform'),
    FieldDef('secChUaMobile', 'Sec-CH-UA-Mobile', '?0 or ?1'),
    FieldDef('secChUaFullVersionList', 'Sec-CH-UA-Full-Version-List',
        'Full version brand list'),
    FieldDef('secChUaPlatformVersion', 'Sec-CH-UA-Platform-Version',
        'Quoted OS version'),
    FieldDef('secChUaArch', 'Sec-CH-UA-Arch', 'Quoted CPU arch'),
    FieldDef('secChUaBitness', 'Sec-CH-UA-Bitness', 'Quoted bitness'),
    FieldDef('secChUaModel', 'Sec-CH-UA-Model', 'Device model (mobile)'),
  ]),

  // ── Language & Locale ──
  CategoryDef('Language & Locale', [
    FieldDef('acceptLanguage', 'Accept-Language', 'HTTP header value'),
    FieldDef('languages', 'navigator.languages', 'Comma-separated'),
    FieldDef('timezone', 'Timezone', 'IANA name, e.g. America/New_York'),
    FieldDef('timezoneOffset', 'Timezone Offset', 'Minutes from UTC'),
    FieldDef('locale', 'Locale', 'e.g. en-US'),
  ]),

  // ── Screen & Display ──
  CategoryDef('Screen & Display', [
    FieldDef('screenWidth', 'screen.width', 'CSS pixels'),
    FieldDef('screenHeight', 'screen.height', 'CSS pixels'),
    FieldDef('availWidth', 'screen.availWidth', 'Minus OS chrome'),
    FieldDef('availHeight', 'screen.availHeight', 'Minus taskbar'),
    FieldDef('innerWidth', 'window.innerWidth', 'Viewport width'),
    FieldDef('innerHeight', 'window.innerHeight', 'Viewport height'),
    FieldDef('outerWidth', 'window.outerWidth', 'Window width'),
    FieldDef('outerHeight', 'window.outerHeight', 'Window height'),
    FieldDef('colorDepth', 'screen.colorDepth', 'Bits per pixel'),
    FieldDef('pixelDepth', 'screen.pixelDepth', 'Usually = colorDepth'),
    FieldDef('devicePixelRatio', 'window.devicePixelRatio', 'DPI scale'),
    FieldDef('orientation', 'screen.orientation.type', 'e.g. landscape-primary'),
  ]),

  // ── Hardware ──
  CategoryDef('Hardware', [
    FieldDef('hardwareConcurrency', 'CPU Cores', 'Logical processors'),
    FieldDef('deviceMemory', 'Device Memory (GB)', 'RAM exposed to JS'),
    FieldDef('maxTouchPoints', 'Max Touch Points', '0 for non-touch'),
    FieldDef('gpuVendor', 'GPU Vendor', 'Graphics manufacturer'),
    FieldDef('gpuModel', 'GPU Model', 'Graphics card/chip'),
    FieldDef('batteryCharging', 'Battery Charging', 'true or false'),
    FieldDef('batteryLevel', 'Battery Level', '0.0 to 1.0'),
    FieldDef('macAddress', 'MAC Address', 'Not web-accessible (spoofed)'),
  ]),

  // ── Network ──
  CategoryDef('Network', [
    FieldDef('connectionType', 'Connection Type', 'wifi, cellular, ethernet'),
    FieldDef(
        'connectionEffectiveType', 'Effective Type', '4g, 3g, 2g, slow-2g'),
    FieldDef('connectionDownlink', 'Downlink (Mbps)', 'Estimated bandwidth'),
    FieldDef('connectionRtt', 'RTT (ms)', 'Round-trip time'),
    FieldDef('connectionSaveData', 'Save Data', 'true or false'),
  ]),

  // ── WebGL & Graphics ──
  CategoryDef('WebGL & Graphics', [
    FieldDef('webglVendor', 'WebGL Vendor', 'WEBGL_debug_renderer_info'),
    FieldDef('webglRenderer', 'WebGL Renderer', 'GPU renderer string'),
    FieldDef('webglVersion', 'WebGL Version', 'e.g. WebGL 2.0'),
    FieldDef('webglShadingVersion', 'Shading Language', 'GLSL version'),
    FieldDef('webglMaxTextureSize', 'Max Texture Size', 'Pixels'),
    FieldDef(
        'webglMaxRenderbufferSize', 'Max Renderbuffer Size', 'Pixels'),
    FieldDef('webglExtensions', 'WebGL Extensions', 'Comma-separated'),
  ]),

  // ── Plugins & MIME Types ──
  CategoryDef('Plugins & MIME Types', [
    FieldDef('plugins', 'Browser Plugins', 'Comma-separated'),
    FieldDef('mimeTypes', 'MIME Types', 'Comma-separated'),
  ]),

  // ── Fonts ──
  CategoryDef('Fonts', [
    FieldDef('fonts', 'Installed Fonts', 'Comma-separated'),
  ]),

  // ── Audio Fingerprint ──
  CategoryDef('Audio Fingerprint', [
    FieldDef('audioSampleRate', 'Sample Rate', 'e.g. 44100, 48000'),
    FieldDef('audioChannelCount', 'Channel Count', 'e.g. 2'),
    FieldDef('audioBaseLatency', 'Base Latency', 'Seconds, e.g. 0.01'),
  ]),

  // ── Privacy ──
  CategoryDef('Privacy', [
    FieldDef('doNotTrack', 'Do Not Track', '1, 0, or null'),
    FieldDef('globalPrivacyControl', 'Global Privacy Control', 'true/false'),
    FieldDef('cookiesEnabled', 'Cookies Enabled', 'true or false'),
  ]),
];

// ── Profile class ───────────────────────────────────────────────

class IdentityProfile {
  String name;
  String description;
  final bool isPreset;
  final Map<String, String> fields;

  IdentityProfile({
    required this.name,
    required this.description,
    this.isPreset = false,
    required Map<String, String> fields,
  }) : fields = Map<String, String>.from(fields);

  String get(String key) => fields[key] ?? '';
  void set(String key, String value) => fields[key] = value;

  // Convenience getters for HTTP-level header injection.
  String get userAgent => fields['userAgent'] ?? '';
  String get acceptLanguage =>
      fields['acceptLanguage'] ?? 'en-US,en;q=0.9';
  String get doNotTrack => fields['doNotTrack'] ?? 'null';
  String get secChUa => fields['secChUa'] ?? '';
  String get secChUaPlatform => fields['secChUaPlatform'] ?? '';
  String get secChUaMobile => fields['secChUaMobile'] ?? '?0';
  String get secChUaFullVersionList =>
      fields['secChUaFullVersionList'] ?? '';
  String get secChUaPlatformVersion =>
      fields['secChUaPlatformVersion'] ?? '';
  String get secChUaArch => fields['secChUaArch'] ?? '';
  String get secChUaBitness => fields['secChUaBitness'] ?? '';
  String get secChUaModel => fields['secChUaModel'] ?? '';

  /// Deep-copy this profile under a new name.
  IdentityProfile clone(String newName) => IdentityProfile(
        name: newName,
        description: 'Custom profile based on $name',
        isPreset: false,
        fields: Map<String, String>.from(fields),
      );

  Map<String, dynamic> toJson() => {
        'name': name,
        'description': description,
        'isPreset': isPreset,
        'fields': fields,
      };

  factory IdentityProfile.fromJson(Map<String, dynamic> json) {
    return IdentityProfile(
      name: json['name'] as String? ?? 'Unknown',
      description: json['description'] as String? ?? '',
      isPreset: json['isPreset'] as bool? ?? false,
      fields: Map<String, String>.from(json['fields'] as Map? ?? {}),
    );
  }

  // ── Preset factories ──────────────────────────────────────────

  /// No spoofing — sends Pane's own identity.
  factory IdentityProfile.none() => IdentityProfile(
        name: 'None (Pane Default)',
        description: 'No identity spoofing — sends default Pane headers',
        isPreset: true,
        fields: {},
      );

  /// Windows 11 / Microsoft Edge 121 desktop.
  factory IdentityProfile.windowsEdge() => IdentityProfile(
        name: 'Windows 11 / Edge 121',
        description: 'Desktop: Windows 11 23H2, Microsoft Edge 121, 1080p',
        isPreset: true,
        fields: {
          // Browser & Engine
          'userAgent':
              'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 '
                  '(KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36 '
                  'Edg/121.0.2277.83',
          'browserName': 'Microsoft Edge',
          'browserVersion': '121.0.2277.83',
          'engineName': 'Blink',
          'engineVersion': '121.0.6167.85',
          'appName': 'Netscape',
          'appVersion':
              '5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 '
                  '(KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36 '
                  'Edg/121.0.2277.83',
          'product': 'Gecko',
          'productSub': '20030107',
          'vendor': 'Google Inc.',
          'vendorSub': '',
          'buildID': '',
          // OS
          'osName': 'Windows',
          'osVersion': '10.0',
          'platform': 'Win32',
          'architecture': 'x86_64',
          'bitness': '64',
          'mobile': 'false',
          'oscpu': '',
          // Client Hints
          'secChUa':
              '"Not A(Brand";v="99", "Microsoft Edge";v="121", "Chromium";v="121"',
          'secChUaPlatform': '"Windows"',
          'secChUaMobile': '?0',
          'secChUaFullVersionList':
              '"Not A(Brand";v="99.0.0.0", "Microsoft Edge";v="121.0.2277.83", '
                  '"Chromium";v="121.0.6167.85"',
          'secChUaPlatformVersion': '"15.0.0"',
          'secChUaArch': '"x86"',
          'secChUaBitness': '"64"',
          'secChUaModel': '""',
          // Language
          'acceptLanguage': 'en-US,en;q=0.9',
          'languages': 'en-US,en',
          'timezone': 'America/New_York',
          'timezoneOffset': '-300',
          'locale': 'en-US',
          // Screen
          'screenWidth': '1920',
          'screenHeight': '1080',
          'availWidth': '1920',
          'availHeight': '1040',
          'innerWidth': '1903',
          'innerHeight': '937',
          'outerWidth': '1920',
          'outerHeight': '1040',
          'colorDepth': '24',
          'pixelDepth': '24',
          'devicePixelRatio': '1.0',
          'orientation': 'landscape-primary',
          // Hardware
          'hardwareConcurrency': '8',
          'deviceMemory': '8',
          'maxTouchPoints': '0',
          'gpuVendor': 'NVIDIA Corporation',
          'gpuModel': 'NVIDIA GeForce GTX 1660 SUPER',
          'batteryCharging': 'true',
          'batteryLevel': '1.0',
          'macAddress': '4C:CC:6A:2B:47:E5',
          // Network
          'connectionType': 'wifi',
          'connectionEffectiveType': '4g',
          'connectionDownlink': '10.0',
          'connectionRtt': '50',
          'connectionSaveData': 'false',
          // WebGL
          'webglVendor': 'Google Inc. (NVIDIA)',
          'webglRenderer':
              'ANGLE (NVIDIA, NVIDIA GeForce GTX 1660 SUPER '
                  'Direct3D11 vs_5_0 ps_5_0, D3D11)',
          'webglVersion': 'WebGL 2.0 (OpenGL ES 3.0 Chromium)',
          'webglShadingVersion':
              'WebGL GLSL ES 3.00 (OpenGL ES GLSL ES 3.0 Chromium)',
          'webglMaxTextureSize': '16384',
          'webglMaxRenderbufferSize': '16384',
          'webglExtensions':
              'ANGLE_instanced_arrays,EXT_blend_minmax,'
                  'EXT_color_buffer_half_float,EXT_float_blend,EXT_frag_depth,'
                  'EXT_shader_texture_lod,EXT_texture_compression_bptc,'
                  'EXT_texture_compression_rgtc,EXT_texture_filter_anisotropic,'
                  'EXT_sRGB,KHR_parallel_shader_compile,'
                  'OES_element_index_uint,OES_fbo_render_mipmap,'
                  'OES_standard_derivatives,OES_texture_float,'
                  'OES_texture_float_linear,OES_texture_half_float,'
                  'OES_texture_half_float_linear,OES_vertex_array_object,'
                  'WEBGL_color_buffer_float,WEBGL_compressed_texture_s3tc,'
                  'WEBGL_compressed_texture_s3tc_srgb,'
                  'WEBGL_debug_renderer_info,WEBGL_debug_shaders,'
                  'WEBGL_depth_texture,WEBGL_draw_buffers,'
                  'WEBGL_lose_context,WEBGL_multi_draw',
          // Plugins
          'plugins':
              'PDF Viewer,Chrome PDF Plugin,Chrome PDF Viewer,'
                  'Microsoft Edge PDF Viewer,WebView',
          'mimeTypes': 'application/pdf,text/pdf',
          // Fonts
          'fonts':
              'Arial,Arial Black,Calibri,Cambria,Cambria Math,'
                  'Comic Sans MS,Consolas,Courier New,Georgia,Impact,'
                  'Lucida Console,Microsoft Sans Serif,Palatino Linotype,'
                  'Segoe UI,Tahoma,Times New Roman,Trebuchet MS,Verdana,'
                  'Wingdings',
          // Audio
          'audioSampleRate': '44100',
          'audioChannelCount': '2',
          'audioBaseLatency': '0.01',
          // Privacy
          'doNotTrack': 'null',
          'globalPrivacyControl': 'false',
          'cookiesEnabled': 'true',
        },
      );

  /// macOS Sonoma / Safari 17 desktop.
  factory IdentityProfile.macSafari() => IdentityProfile(
        name: 'macOS Sonoma / Safari 17',
        description: 'Desktop: macOS 14.2, Safari 17.2, Retina display',
        isPreset: true,
        fields: {
          'userAgent':
              'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) '
                  'AppleWebKit/605.1.15 (KHTML, like Gecko) '
                  'Version/17.2.1 Safari/605.1.15',
          'browserName': 'Safari',
          'browserVersion': '17.2.1',
          'engineName': 'WebKit',
          'engineVersion': '605.1.15',
          'appName': 'Netscape',
          'appVersion':
              '5.0 (Macintosh; Intel Mac OS X 10_15_7) '
                  'AppleWebKit/605.1.15 (KHTML, like Gecko) '
                  'Version/17.2.1 Safari/605.1.15',
          'product': 'Gecko',
          'productSub': '20030107',
          'vendor': 'Apple Computer, Inc.',
          'vendorSub': '',
          'buildID': '',
          'osName': 'macOS',
          'osVersion': '14.2',
          'platform': 'MacIntel',
          'architecture': 'arm64',
          'bitness': '64',
          'mobile': 'false',
          'oscpu': '',
          // Safari does not send Client Hints.
          'secChUa': '',
          'secChUaPlatform': '',
          'secChUaMobile': '',
          'secChUaFullVersionList': '',
          'secChUaPlatformVersion': '',
          'secChUaArch': '',
          'secChUaBitness': '',
          'secChUaModel': '',
          'acceptLanguage': 'en-US,en;q=0.9',
          'languages': 'en-US',
          'timezone': 'America/Los_Angeles',
          'timezoneOffset': '-480',
          'locale': 'en-US',
          'screenWidth': '2560',
          'screenHeight': '1440',
          'availWidth': '2560',
          'availHeight': '1415',
          'innerWidth': '2560',
          'innerHeight': '1337',
          'outerWidth': '2560',
          'outerHeight': '1415',
          'colorDepth': '30',
          'pixelDepth': '30',
          'devicePixelRatio': '2.0',
          'orientation': 'landscape-primary',
          'hardwareConcurrency': '10',
          'deviceMemory': '',
          'maxTouchPoints': '0',
          'gpuVendor': 'Apple',
          'gpuModel': 'Apple M2 Pro',
          'batteryCharging': '',
          'batteryLevel': '',
          'macAddress': '3C:22:FB:A1:9D:04',
          'connectionType': '',
          'connectionEffectiveType': '',
          'connectionDownlink': '',
          'connectionRtt': '',
          'connectionSaveData': 'false',
          'webglVendor': 'Apple Inc.',
          'webglRenderer': 'Apple M2 Pro',
          'webglVersion': 'WebGL 2.0',
          'webglShadingVersion': 'WebGL GLSL ES 3.00',
          'webglMaxTextureSize': '16384',
          'webglMaxRenderbufferSize': '16384',
          'webglExtensions':
              'EXT_blend_minmax,EXT_color_buffer_float,'
                  'EXT_color_buffer_half_float,EXT_float_blend,'
                  'EXT_texture_filter_anisotropic,EXT_sRGB,'
                  'OES_element_index_uint,OES_fbo_render_mipmap,'
                  'OES_standard_derivatives,OES_texture_float,'
                  'OES_texture_float_linear,OES_texture_half_float,'
                  'OES_texture_half_float_linear,OES_vertex_array_object,'
                  'WEBGL_color_buffer_float,WEBGL_compressed_texture_astc,'
                  'WEBGL_compressed_texture_etc,WEBGL_depth_texture,'
                  'WEBGL_draw_buffers,WEBGL_lose_context',
          'plugins': '',
          'mimeTypes': '',
          'fonts':
              'Arial,Arial Black,Courier New,Georgia,Helvetica,'
                  'Helvetica Neue,Lucida Grande,Menlo,Monaco,SF Pro,'
                  'SF Pro Display,SF Mono,Times New Roman,Trebuchet MS,'
                  'Verdana',
          'audioSampleRate': '44100',
          'audioChannelCount': '2',
          'audioBaseLatency': '0.0029',
          'doNotTrack': 'null',
          'globalPrivacyControl': 'false',
          'cookiesEnabled': 'true',
        },
      );

  /// Ubuntu 22.04 / Firefox 121 desktop.
  factory IdentityProfile.linuxFirefox() => IdentityProfile(
        name: 'Ubuntu 22.04 / Firefox 121',
        description: 'Desktop: Linux, Firefox 121, 1080p display',
        isPreset: true,
        fields: {
          'userAgent':
              'Mozilla/5.0 (X11; Linux x86_64; rv:121.0) '
                  'Gecko/20100101 Firefox/121.0',
          'browserName': 'Firefox',
          'browserVersion': '121.0',
          'engineName': 'Gecko',
          'engineVersion': '121.0',
          'appName': 'Netscape',
          'appVersion': '5.0 (X11)',
          'product': 'Gecko',
          'productSub': '20100101',
          'vendor': '',
          'vendorSub': '',
          'buildID': '20240108143603',
          'osName': 'Linux',
          'osVersion': '22.04',
          'platform': 'Linux x86_64',
          'architecture': 'x86_64',
          'bitness': '64',
          'mobile': 'false',
          'oscpu': 'Linux x86_64',
          // Firefox does not send Client Hints.
          'secChUa': '',
          'secChUaPlatform': '',
          'secChUaMobile': '',
          'secChUaFullVersionList': '',
          'secChUaPlatformVersion': '',
          'secChUaArch': '',
          'secChUaBitness': '',
          'secChUaModel': '',
          'acceptLanguage': 'en-US,en;q=0.5',
          'languages': 'en-US,en',
          'timezone': 'Europe/London',
          'timezoneOffset': '0',
          'locale': 'en-US',
          'screenWidth': '1920',
          'screenHeight': '1080',
          'availWidth': '1920',
          'availHeight': '1053',
          'innerWidth': '1848',
          'innerHeight': '953',
          'outerWidth': '1920',
          'outerHeight': '1053',
          'colorDepth': '24',
          'pixelDepth': '24',
          'devicePixelRatio': '1.0',
          'orientation': 'landscape-primary',
          'hardwareConcurrency': '4',
          'deviceMemory': '',
          'maxTouchPoints': '0',
          'gpuVendor': 'Intel',
          'gpuModel': 'Intel UHD Graphics 630',
          'batteryCharging': '',
          'batteryLevel': '',
          'macAddress': '08:00:27:5B:3D:F9',
          'connectionType': 'ethernet',
          'connectionEffectiveType': '',
          'connectionDownlink': '',
          'connectionRtt': '',
          'connectionSaveData': 'false',
          'webglVendor': 'Intel',
          'webglRenderer':
              'Mesa Intel(R) UHD Graphics 630 (CFL GT2)',
          'webglVersion': 'WebGL 2.0',
          'webglShadingVersion': 'WebGL GLSL ES 3.00',
          'webglMaxTextureSize': '16384',
          'webglMaxRenderbufferSize': '16384',
          'webglExtensions':
              'ANGLE_instanced_arrays,EXT_blend_minmax,'
                  'EXT_color_buffer_half_float,EXT_float_blend,'
                  'EXT_texture_filter_anisotropic,EXT_frag_depth,'
                  'EXT_shader_texture_lod,EXT_sRGB,'
                  'OES_element_index_uint,OES_standard_derivatives,'
                  'OES_texture_float,OES_texture_float_linear,'
                  'OES_texture_half_float,OES_texture_half_float_linear,'
                  'OES_vertex_array_object,WEBGL_color_buffer_float,'
                  'WEBGL_depth_texture,WEBGL_draw_buffers,'
                  'WEBGL_lose_context',
          'plugins': '',
          'mimeTypes': 'application/pdf',
          'fonts':
              'Arial,Courier New,DejaVu Sans,DejaVu Sans Mono,'
                  'DejaVu Serif,Droid Sans,Droid Sans Mono,FreeMono,'
                  'FreeSans,FreeSerif,Liberation Mono,Liberation Sans,'
                  'Liberation Serif,Noto Sans,Noto Serif,Times New Roman,'
                  'Ubuntu,Ubuntu Mono',
          'audioSampleRate': '44100',
          'audioChannelCount': '2',
          'audioBaseLatency': '0.005',
          'doNotTrack': 'unspecified',
          'globalPrivacyControl': 'false',
          'cookiesEnabled': 'true',
        },
      );

  /// Android 14 / Chrome 121 mobile.
  factory IdentityProfile.androidChrome() => IdentityProfile(
        name: 'Android 14 / Chrome 121',
        description: 'Mobile: Pixel 8 Pro, Chrome 121, Android 14',
        isPreset: true,
        fields: {
          'userAgent':
              'Mozilla/5.0 (Linux; Android 14; Pixel 8 Pro) '
                  'AppleWebKit/537.36 (KHTML, like Gecko) '
                  'Chrome/121.0.6167.101 Mobile Safari/537.36',
          'browserName': 'Chrome Mobile',
          'browserVersion': '121.0.6167.101',
          'engineName': 'Blink',
          'engineVersion': '121.0.6167.101',
          'appName': 'Netscape',
          'appVersion':
              '5.0 (Linux; Android 14; Pixel 8 Pro) '
                  'AppleWebKit/537.36 (KHTML, like Gecko) '
                  'Chrome/121.0.6167.101 Mobile Safari/537.36',
          'product': 'Gecko',
          'productSub': '20030107',
          'vendor': 'Google Inc.',
          'vendorSub': '',
          'buildID': '',
          'osName': 'Android',
          'osVersion': '14',
          'platform': 'Linux armv81',
          'architecture': 'armv81',
          'bitness': '64',
          'mobile': 'true',
          'oscpu': '',
          'secChUa':
              '"Not A(Brand";v="99", "Google Chrome";v="121", "Chromium";v="121"',
          'secChUaPlatform': '"Android"',
          'secChUaMobile': '?1',
          'secChUaFullVersionList':
              '"Not A(Brand";v="99.0.0.0", "Google Chrome";v="121.0.6167.101", '
                  '"Chromium";v="121.0.6167.101"',
          'secChUaPlatformVersion': '"14.0.0"',
          'secChUaArch': '"arm"',
          'secChUaBitness': '"64"',
          'secChUaModel': '"Pixel 8 Pro"',
          'acceptLanguage': 'en-US,en;q=0.9',
          'languages': 'en-US,en',
          'timezone': 'America/Chicago',
          'timezoneOffset': '-360',
          'locale': 'en-US',
          'screenWidth': '412',
          'screenHeight': '915',
          'availWidth': '412',
          'availHeight': '891',
          'innerWidth': '412',
          'innerHeight': '780',
          'outerWidth': '412',
          'outerHeight': '891',
          'colorDepth': '24',
          'pixelDepth': '24',
          'devicePixelRatio': '2.625',
          'orientation': 'portrait-primary',
          'hardwareConcurrency': '8',
          'deviceMemory': '8',
          'maxTouchPoints': '5',
          'gpuVendor': 'Qualcomm',
          'gpuModel': 'Adreno (TM) 750',
          'batteryCharging': 'false',
          'batteryLevel': '0.72',
          'macAddress': 'A4:77:33:1E:8C:B2',
          'connectionType': 'cellular',
          'connectionEffectiveType': '4g',
          'connectionDownlink': '3.45',
          'connectionRtt': '100',
          'connectionSaveData': 'false',
          'webglVendor': 'Qualcomm',
          'webglRenderer': 'Adreno (TM) 750',
          'webglVersion': 'WebGL 2.0 (OpenGL ES 3.0 Chromium)',
          'webglShadingVersion':
              'WebGL GLSL ES 3.00 (OpenGL ES GLSL ES 3.0 Chromium)',
          'webglMaxTextureSize': '16384',
          'webglMaxRenderbufferSize': '16384',
          'webglExtensions':
              'ANGLE_instanced_arrays,EXT_blend_minmax,'
                  'EXT_color_buffer_half_float,EXT_float_blend,'
                  'EXT_texture_filter_anisotropic,EXT_sRGB,'
                  'OES_element_index_uint,OES_standard_derivatives,'
                  'OES_texture_float,OES_texture_float_linear,'
                  'OES_texture_half_float,OES_texture_half_float_linear,'
                  'OES_vertex_array_object,WEBGL_color_buffer_float,'
                  'WEBGL_compressed_texture_astc,'
                  'WEBGL_compressed_texture_etc,'
                  'WEBGL_depth_texture,WEBGL_draw_buffers,'
                  'WEBGL_lose_context',
          'plugins': '',
          'mimeTypes': '',
          'fonts':
              'Droid Sans,Droid Sans Mono,Droid Serif,Noto Color Emoji,'
                  'Noto Sans,Roboto,Roboto Mono',
          'audioSampleRate': '48000',
          'audioChannelCount': '2',
          'audioBaseLatency': '0.01',
          'doNotTrack': 'null',
          'globalPrivacyControl': 'false',
          'cookiesEnabled': 'true',
        },
      );

  /// iPhone 15 / Safari 17 mobile.
  factory IdentityProfile.iphoneSafari() => IdentityProfile(
        name: 'iPhone 15 / Safari 17',
        description: 'Mobile: iPhone 15, Safari 17.2, iOS 17.2',
        isPreset: true,
        fields: {
          'userAgent':
              'Mozilla/5.0 (iPhone; CPU iPhone OS 17_2_1 like Mac OS X) '
                  'AppleWebKit/605.1.15 (KHTML, like Gecko) '
                  'Version/17.2 Mobile/15E148 Safari/604.1',
          'browserName': 'Mobile Safari',
          'browserVersion': '17.2',
          'engineName': 'WebKit',
          'engineVersion': '605.1.15',
          'appName': 'Netscape',
          'appVersion':
              '5.0 (iPhone; CPU iPhone OS 17_2_1 like Mac OS X) '
                  'AppleWebKit/605.1.15 (KHTML, like Gecko) '
                  'Version/17.2 Mobile/15E148 Safari/604.1',
          'product': 'Gecko',
          'productSub': '20030107',
          'vendor': 'Apple Computer, Inc.',
          'vendorSub': '',
          'buildID': '',
          'osName': 'iOS',
          'osVersion': '17.2.1',
          'platform': 'iPhone',
          'architecture': 'arm64',
          'bitness': '64',
          'mobile': 'true',
          'oscpu': '',
          // Safari does not send Client Hints.
          'secChUa': '',
          'secChUaPlatform': '',
          'secChUaMobile': '',
          'secChUaFullVersionList': '',
          'secChUaPlatformVersion': '',
          'secChUaArch': '',
          'secChUaBitness': '',
          'secChUaModel': '',
          'acceptLanguage': 'en-US,en;q=0.9',
          'languages': 'en-US',
          'timezone': 'America/Denver',
          'timezoneOffset': '-420',
          'locale': 'en-US',
          'screenWidth': '393',
          'screenHeight': '852',
          'availWidth': '393',
          'availHeight': '852',
          'innerWidth': '393',
          'innerHeight': '660',
          'outerWidth': '393',
          'outerHeight': '852',
          'colorDepth': '32',
          'pixelDepth': '32',
          'devicePixelRatio': '3.0',
          'orientation': 'portrait-primary',
          'hardwareConcurrency': '6',
          'deviceMemory': '',
          'maxTouchPoints': '5',
          'gpuVendor': 'Apple Inc.',
          'gpuModel': 'Apple GPU',
          'batteryCharging': 'false',
          'batteryLevel': '0.64',
          'macAddress': 'F8:4D:89:67:2A:C1',
          'connectionType': 'wifi',
          'connectionEffectiveType': '',
          'connectionDownlink': '',
          'connectionRtt': '',
          'connectionSaveData': 'false',
          'webglVendor': 'Apple Inc.',
          'webglRenderer': 'Apple GPU',
          'webglVersion': 'WebGL 2.0',
          'webglShadingVersion': 'WebGL GLSL ES 3.00',
          'webglMaxTextureSize': '16384',
          'webglMaxRenderbufferSize': '16384',
          'webglExtensions':
              'EXT_blend_minmax,EXT_color_buffer_float,'
                  'EXT_color_buffer_half_float,EXT_float_blend,'
                  'EXT_texture_filter_anisotropic,EXT_sRGB,'
                  'OES_element_index_uint,OES_standard_derivatives,'
                  'OES_texture_float,OES_texture_float_linear,'
                  'OES_texture_half_float,OES_texture_half_float_linear,'
                  'OES_vertex_array_object,WEBGL_color_buffer_float,'
                  'WEBGL_compressed_texture_astc,'
                  'WEBGL_compressed_texture_etc,'
                  'WEBGL_depth_texture,WEBGL_draw_buffers,'
                  'WEBGL_lose_context',
          'plugins': '',
          'mimeTypes': '',
          'fonts':
              'Arial,Courier New,Georgia,Helvetica,Helvetica Neue,'
                  'SF Pro,SF Pro Display,SF Compact,Times New Roman,'
                  'Trebuchet MS,Verdana',
          'audioSampleRate': '48000',
          'audioChannelCount': '2',
          'audioBaseLatency': '0.005',
          'doNotTrack': 'null',
          'globalPrivacyControl': 'false',
          'cookiesEnabled': 'true',
        },
      );

  /// All built-in preset profiles.
  static List<IdentityProfile> get presets => [
        IdentityProfile.none(),
        IdentityProfile.windowsEdge(),
        IdentityProfile.macSafari(),
        IdentityProfile.linuxFirefox(),
        IdentityProfile.androidChrome(),
        IdentityProfile.iphoneSafari(),
      ];
}
