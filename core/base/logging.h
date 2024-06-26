#pragma once

#include <optional>

#pragma warning(push)
#pragma warning(disable : 4505)
#include "aixlog.hpp"
#pragma warning(pop)

namespace foxglove {

typedef AixLog::Severity LogLevel;

struct LogConfig {
  std::optional<bool> enable_console_logging;
  std::optional<LogLevel> console_log_level;
  std::optional<std::string> file_log_path;
  std::optional<LogLevel> file_log_level;
};

inline LogLevel GetLogLevel(int level) {
  switch (level) {
    case 1:
      return AixLog::Severity::debug;
    case 2:
      return AixLog::Severity::info;
    case 3:
      return AixLog::Severity::warning;
    case 4:
      return AixLog::Severity::error;
    case 5:
      return AixLog::Severity::fatal;
    default:
      return AixLog::Severity::trace;
  }
}

void SetupLogging();
void SetupLogging(const LogConfig& config);

}  // namespace foxglove
