#include "logging.h"

#include <filesystem>
#include <iostream>

namespace foxglove {
namespace windows {

void SetupLogging() {
  AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
}

void SetupLogging(const LogConfig& config) {
  std::vector<std::shared_ptr<AixLog::Sink>> log_sinks;

  if (config.enable_console_logging.value_or(false)) {
    auto level = config.console_log_level.value_or(LogLevel::trace);
    log_sinks.emplace_back(std::make_shared<AixLog::SinkCerr>(level));
  }

  if (config.file_log_path.has_value()) {
    auto level = config.file_log_level.value_or(LogLevel::info);

    auto path = std::filesystem::path(config.file_log_path.value());
    auto log_directory = path.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(log_directory, ec);

    log_sinks.emplace_back(std::make_shared<AixLog::SinkFile>(
        level, config.file_log_path.value()));
  }
  AixLog::Log::init(log_sinks);
}

}  // namespace windows
}  // namespace foxglove
