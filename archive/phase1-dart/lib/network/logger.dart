/// Simple file logger for Pane browser errors.
///
/// Appends timestamped entries to a log file in the user's home directory.
/// The log file is created on first write and rotated when it exceeds 1 MB.

import 'dart:io';

class PaneLogger {
  static File? _logFile;
  static const int _maxLogSize = 1024 * 1024; // 1 MB

  static File get _file {
    if (_logFile != null) return _logFile!;
    final home = Platform.environment['HOME'] ??
        Platform.environment['USERPROFILE'] ??
        '.';
    _logFile = File('$home/pane_browser.log');
    return _logFile!;
  }

  static String get logPath => _file.path;

  /// Log an error with context about what was happening.
  static void error(String context, Object error, [StackTrace? stack]) {
    final timestamp = DateTime.now().toIso8601String();
    final buf = StringBuffer()
      ..writeln('[$timestamp] ERROR — $context')
      ..writeln('  $error');
    if (stack != null) {
      // Keep only the first 8 frames to avoid huge logs.
      final frames = stack.toString().split('\n').take(8).join('\n  ');
      buf.writeln('  $frames');
    }
    buf.writeln();
    _append(buf.toString());
  }

  /// Log a warning (non-fatal issue).
  static void warn(String context, String message) {
    final timestamp = DateTime.now().toIso8601String();
    _append('[$timestamp] WARN  — $context: $message\n');
  }

  /// Log an informational message.
  static void info(String message) {
    final timestamp = DateTime.now().toIso8601String();
    _append('[$timestamp] INFO  — $message\n');
  }

  static void _append(String text) {
    try {
      final file = _file;
      // Rotate if too large.
      if (file.existsSync() && file.lengthSync() > _maxLogSize) {
        final old = File('${file.path}.old');
        if (old.existsSync()) old.deleteSync();
        file.renameSync(old.path);
        _logFile = File(file.path);
      }
      _file.writeAsStringSync(text, mode: FileMode.append, flush: true);
    } catch (_) {
      // If we can't write the log, don't crash the browser.
    }
  }
}
