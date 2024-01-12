#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <optional>
#include <string>

namespace foxglove {

class ErrorDetails {
 public:
  ErrorDetails() = delete;
  ErrorDetails(std::string_view message, std::string_view description,
               int error_code)
      : message_(message), description_(description), error_code_(error_code) {}
  ErrorDetails(std::string_view message, int error_code)
      : message_(message), error_code_(error_code) {}
  ErrorDetails(std::string_view message) : message_(message) {}

#ifdef _WIN32
  static ErrorDetails FromHResult(HRESULT hr, std::string_view message);
#endif

  constexpr std::optional<int> code() const { return error_code_; }
  const std::string& message() const { return message_; }
  const std::optional<std::string>& description() const { return description_; }

  std::string ToString() const;
  std::string CodeToString() const;

 private:
  const std::string message_;
  const std::optional<std::string> description_;
  const std::optional<int> error_code_;
};

}  // namespace foxglove
