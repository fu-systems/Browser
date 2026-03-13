/// Plugin manifest — metadata and capability declarations.
///
/// Each plugin declares its name, version, description, author, and the
/// set of capabilities it requires. The registry validates capabilities
/// before enabling a plugin.

/// Known capability identifiers.
///
/// Plugins declare which hooks they intend to use so the registry can
/// present meaningful permission prompts and restrict access.
class PluginCapability {
  static const network = 'network'; // onBeforeRequest, onAfterResponse
  static const dom = 'dom'; // onDomReady
  static const style = 'style'; // onStylesComputed
  static const layout = 'layout'; // onLayoutComplete
  static const paint = 'paint'; // onPaint
  static const navigation = 'navigation'; // onNavigate, onLinkClick
  static const cookies = 'cookies'; // cookie control via PluginContext
  static const headers = 'headers'; // custom header injection
}

class PluginManifest {
  final String name;
  final String version;
  final String description;
  final String author;
  final Set<String> capabilities;

  const PluginManifest({
    required this.name,
    required this.version,
    required this.description,
    required this.author,
    required this.capabilities,
  });

  /// Parse from a Map (e.g., decoded YAML/JSON).
  factory PluginManifest.fromMap(Map<String, dynamic> map) {
    return PluginManifest(
      name: map['name'] as String? ?? '',
      version: map['version'] as String? ?? '0.0.0',
      description: map['description'] as String? ?? '',
      author: map['author'] as String? ?? '',
      capabilities: (map['capabilities'] as List<dynamic>?)
              ?.map((e) => e.toString())
              .toSet() ??
          {},
    );
  }
}
