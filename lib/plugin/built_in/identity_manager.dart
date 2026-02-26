/// Identity Manager — built-in plugin for browser fingerprint spoofing.
///
/// Applies the active [IdentityProfile] to every outgoing HTTP request
/// by setting User-Agent, Accept-Language, Client Hints, DNT, and other
/// headers. Persists the active profile and any custom profiles to
/// `~/.pane_identity_profiles.json`.

import 'dart:io';
import 'dart:convert';

import '../plugin.dart';
import '../../network/logger.dart';
import 'identity_profile.dart';

class IdentityManagerPlugin extends Plugin {
  @override
  String get name => 'identity_manager';

  /// The profile currently applied to outgoing requests.
  /// When [activeProfile.fields] is empty (the "None" preset) no headers
  /// are spoofed and Pane's defaults are used.
  IdentityProfile activeProfile = IdentityProfile.none();

  /// User-created profiles persisted to disk.
  List<IdentityProfile> customProfiles = [];

  File? _file;

  File get _storageFile {
    if (_file != null) return _file!;
    final home = Platform.environment['HOME'] ??
        Platform.environment['USERPROFILE'] ??
        '.';
    _file = File('$home/.pane_identity_profiles.json');
    return _file!;
  }

  @override
  Future<void> onEnable() async {
    _loadFromDisk();
  }

  // ── Network hook ──────────────────────────────────────────────

  @override
  FetchRequest? onBeforeRequest(FetchRequest request) {
    final p = activeProfile;
    if (p.fields.isEmpty) return request; // "None" — no spoofing.

    // User-Agent.
    if (p.userAgent.isNotEmpty) {
      request.headers['User-Agent'] = p.userAgent;
    }

    // Accept-Language.
    if (p.acceptLanguage.isNotEmpty) {
      request.headers['Accept-Language'] = p.acceptLanguage;
    }

    // Do Not Track.
    final dnt = p.doNotTrack;
    if (dnt == '1') {
      request.headers['DNT'] = '1';
    } else if (dnt == '0') {
      request.headers['DNT'] = '0';
    } else {
      request.headers.remove('DNT');
    }

    // Client Hints — only Chromium-based browsers send these.
    _setIfNotEmpty(request.headers, 'Sec-CH-UA', p.secChUa);
    _setIfNotEmpty(request.headers, 'Sec-CH-UA-Platform', p.secChUaPlatform);
    _setIfNotEmpty(request.headers, 'Sec-CH-UA-Mobile', p.secChUaMobile);
    _setIfNotEmpty(
        request.headers, 'Sec-CH-UA-Full-Version-List', p.secChUaFullVersionList);
    _setIfNotEmpty(
        request.headers, 'Sec-CH-UA-Platform-Version', p.secChUaPlatformVersion);
    _setIfNotEmpty(request.headers, 'Sec-CH-UA-Arch', p.secChUaArch);
    _setIfNotEmpty(request.headers, 'Sec-CH-UA-Bitness', p.secChUaBitness);
    _setIfNotEmpty(request.headers, 'Sec-CH-UA-Model', p.secChUaModel);

    return request;
  }

  void _setIfNotEmpty(Map<String, String> headers, String key, String value) {
    if (value.isNotEmpty) {
      headers[key] = value;
    } else {
      // Non-Chromium browsers shouldn't send these at all.
      headers.remove(key);
    }
  }

  // ── Profile management ────────────────────────────────────────

  /// Switch to one of the preset profiles by name.
  void selectPreset(String presetName) {
    final preset = IdentityProfile.presets
        .where((p) => p.name == presetName)
        .firstOrNull;
    if (preset != null) {
      activeProfile = preset;
      _saveToDisk();
    }
  }

  /// Switch to a saved custom profile by name.
  void selectCustom(String customName) {
    final custom =
        customProfiles.where((p) => p.name == customName).firstOrNull;
    if (custom != null) {
      activeProfile = custom;
      _saveToDisk();
    }
  }

  /// Save the active profile as a new custom profile.
  void saveAsCustom(String name) {
    // Remove any existing custom with the same name.
    customProfiles.removeWhere((p) => p.name == name);
    final clone = activeProfile.clone(name);
    customProfiles.add(clone);
    activeProfile = clone;
    _saveToDisk();
  }

  /// Delete a custom profile by name.
  void deleteCustom(String name) {
    customProfiles.removeWhere((p) => p.name == name);
    if (activeProfile.name == name) {
      activeProfile = IdentityProfile.none();
    }
    _saveToDisk();
  }

  /// Update a field in the active profile.
  void updateField(String key, String value) {
    activeProfile.set(key, value);
    _saveToDisk();
  }

  /// All available profiles (presets + custom).
  List<IdentityProfile> get allProfiles => [
        ...IdentityProfile.presets,
        ...customProfiles,
      ];

  // ── Persistence ───────────────────────────────────────────────

  void _loadFromDisk() {
    try {
      final file = _storageFile;
      if (!file.existsSync()) return;
      final content = file.readAsStringSync();
      if (content.trim().isEmpty) return;

      final json = jsonDecode(content) as Map<String, dynamic>;

      // Load custom profiles.
      final customs = json['customProfiles'] as List<dynamic>? ?? [];
      customProfiles = customs
          .map((e) => IdentityProfile.fromJson(e as Map<String, dynamic>))
          .toList();

      // Restore active profile.
      final activeName = json['activeProfileName'] as String? ?? '';
      final activeIsCustom = json['activeIsCustom'] as bool? ?? false;

      if (activeIsCustom) {
        activeProfile = customProfiles
                .where((p) => p.name == activeName)
                .firstOrNull ??
            IdentityProfile.none();
      } else {
        activeProfile = IdentityProfile.presets
                .where((p) => p.name == activeName)
                .firstOrNull ??
            IdentityProfile.none();
      }
    } catch (e) {
      PaneLogger.warn('identity_manager', 'Failed to load profiles: $e');
    }
  }

  void _saveToDisk() {
    try {
      _storageFile.writeAsStringSync(
        jsonEncode({
          'activeProfileName': activeProfile.name,
          'activeIsCustom': !activeProfile.isPreset,
          'customProfiles': customProfiles.map((p) => p.toJson()).toList(),
        }),
        flush: true,
      );
    } catch (e) {
      PaneLogger.warn('identity_manager', 'Failed to save profiles: $e');
    }
  }

  /// Persist current state (called by editor after field edits).
  void save() => _saveToDisk();
}
