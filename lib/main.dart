import 'package:flutter/material.dart';
import 'plugin/plugin_context.dart';
import 'plugin/plugin_manifest.dart';
import 'plugin/plugin_pipeline.dart';
import 'plugin/plugin_registry.dart';
import 'plugin/built_in/dark_mode.dart';
import 'plugin/built_in/privacy_shield.dart';
import 'ui/browser.dart';

void main() {
  // Set up the plugin system.
  final registry = PluginRegistry();
  final context = PluginContext();

  // Register built-in plugins.
  registry.register(
    PrivacyShieldPlugin(context),
    const PluginManifest(
      name: 'privacy_shield',
      version: '1.0.0',
      description: 'Strips tracking parameters and blocks known tracker domains.',
      author: 'Pane',
      capabilities: {PluginCapability.network, PluginCapability.navigation},
    ),
  );

  registry.register(
    DarkModePlugin(),
    const PluginManifest(
      name: 'dark_mode',
      version: '1.0.0',
      description: 'Injects dark-mode CSS into every page.',
      author: 'Pane',
      capabilities: {PluginCapability.dom},
    ),
  );

  // Built-ins start disabled — user enables via settings.
  final pipeline = PluginPipeline(registry, context);

  runApp(PaneApp(pipeline: pipeline, registry: registry));
}

class PaneApp extends StatelessWidget {
  final PluginPipeline pipeline;
  final PluginRegistry registry;

  const PaneApp({super.key, required this.pipeline, required this.registry});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Pane',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.blueGrey,
          brightness: Brightness.light,
        ),
        useMaterial3: true,
      ),
      home: BrowserShell(pluginPipeline: pipeline),
    );
  }
}
