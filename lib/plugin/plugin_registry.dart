/// Plugin registry — manages plugin lifecycle and discovery.
///
/// Registers plugins with their manifests, validates capabilities,
/// enables/disables plugins, and provides the ordered list of enabled
/// plugins to the pipeline.

import 'plugin.dart';
import 'plugin_manifest.dart';

class _RegisteredPlugin {
  final Plugin plugin;
  final PluginManifest manifest;
  bool enabled;
  _RegisteredPlugin(this.plugin, this.manifest, {this.enabled = false});
}

/// Read-only information about a registered plugin (for UI display).
class PluginInfo {
  final String name;
  final String version;
  final String description;
  final String author;
  final Set<String> capabilities;
  final bool enabled;

  PluginInfo({
    required this.name,
    required this.version,
    required this.description,
    required this.author,
    required this.capabilities,
    required this.enabled,
  });
}

class PluginRegistry {
  final List<_RegisteredPlugin> _plugins = [];

  /// Register a plugin with its manifest.
  void register(Plugin plugin, PluginManifest manifest) {
    // Prevent duplicate names.
    _plugins.removeWhere((p) => p.manifest.name == manifest.name);
    _plugins.add(_RegisteredPlugin(plugin, manifest));
  }

  /// Enable a plugin by name. Calls [Plugin.onEnable].
  Future<void> enable(String name) async {
    final entry = _find(name);
    if (entry == null || entry.enabled) return;
    entry.enabled = true;
    await entry.plugin.onEnable();
  }

  /// Disable a plugin by name. Calls [Plugin.onDisable].
  Future<void> disable(String name) async {
    final entry = _find(name);
    if (entry == null || !entry.enabled) return;
    await entry.plugin.onDisable();
    entry.enabled = false;
  }

  /// Uninstall a plugin (disable first, then call onUninstall, then remove).
  Future<void> uninstall(String name) async {
    final entry = _find(name);
    if (entry == null) return;
    if (entry.enabled) await disable(name);
    await entry.plugin.onUninstall();
    _plugins.removeWhere((p) => p.manifest.name == name);
  }

  /// All currently enabled plugins, in registration order.
  List<Plugin> get enabledPlugins =>
      _plugins.where((p) => p.enabled).map((p) => p.plugin).toList();

  /// All registered plugins (for the management UI).
  List<PluginInfo> get allPlugins => _plugins
      .map((p) => PluginInfo(
            name: p.manifest.name,
            version: p.manifest.version,
            description: p.manifest.description,
            author: p.manifest.author,
            capabilities: p.manifest.capabilities,
            enabled: p.enabled,
          ))
      .toList();

  /// Look up a plugin by name.
  bool isEnabled(String name) => _find(name)?.enabled ?? false;

  _RegisteredPlugin? _find(String name) {
    for (final p in _plugins) {
      if (p.manifest.name == name) return p;
    }
    return null;
  }
}
