#pragma once

#include <optional>

namespace foxglove {

struct OkStatus final {
 public:
  constexpr OkStatus() {}
};

template <typename TErr>
class [[nodiscard]] Status final {
 public:
  Status() = delete;
  constexpr Status(TErr error) : error_(error) {}
  constexpr Status(OkStatus) {}

  [[nodiscard]] constexpr bool ok() const { return !error_.has_value(); }

  [[nodiscard]] constexpr bool has_error() const { return !ok(); }

  [[nodiscard]] constexpr const TErr& error() const { return error_.value(); }

  // Ignores any errors. This method does nothing except potentially suppress
  // complaints from any tools that are checking that errors are not dropped on
  // the floor.
  inline constexpr void IgnoreError() const {}

  inline constexpr operator bool() const { return ok(); }

 private:
  const std::optional<TErr> error_;
};

}  // namespace foxglove
