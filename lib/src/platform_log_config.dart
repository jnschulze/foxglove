// Must be kept in sync with native-side enum.
enum PlatformLogLevel { trace, debug, info, warning, error, fatal }

class PlatformLogConfig {
  final bool? enableConsoleLogging;
  final PlatformLogLevel? consoleLogLevel;
  final String? fileLogPath;
  final PlatformLogLevel? fileLogLevel;

  const PlatformLogConfig(
      {this.enableConsoleLogging,
      this.consoleLogLevel,
      this.fileLogPath,
      this.fileLogLevel});

  PlatformLogConfig copyWith({
    bool? enableConsoleLogging,
    PlatformLogLevel? consoleLogLevel,
    String? fileLogPath,
    PlatformLogLevel? fileLogLevel,
  }) =>
      PlatformLogConfig(
        enableConsoleLogging: enableConsoleLogging ?? this.enableConsoleLogging,
        consoleLogLevel: consoleLogLevel ?? this.consoleLogLevel,
        fileLogPath: fileLogPath ?? this.fileLogPath,
        fileLogLevel: fileLogLevel ?? this.fileLogLevel,
      );

  @override
  int get hashCode => Object.hash(
      enableConsoleLogging, consoleLogLevel, fileLogPath, fileLogLevel);

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is PlatformLogConfig &&
        other.enableConsoleLogging == enableConsoleLogging &&
        other.consoleLogLevel == consoleLogLevel &&
        other.fileLogPath == fileLogPath &&
        other.fileLogLevel == fileLogLevel;
  }
}
