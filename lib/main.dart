import 'package:flutter/material.dart';
import 'ui/browser.dart';

void main() {
  runApp(const PaneApp());
}

class PaneApp extends StatelessWidget {
  const PaneApp({super.key});

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
      home: const BrowserShell(),
    );
  }
}
