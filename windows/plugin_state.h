#pragma once

#include <atomic>

namespace foxglove {
namespace windows {
class PluginState {
 public:
  static bool IsValid() { return !is_terminating_; }
  static void SetIsTerminating() { is_terminating_ = true; }

 private:
  static inline std::atomic<bool> is_terminating_{false};
};
}  // namespace windows
}  // namespace foxglove