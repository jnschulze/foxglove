#pragma once

#include <cstdint>

namespace foxglove {
class MediaInfo {
 public:
  MediaInfo(int64_t duration) : duration_(duration) {}

  // The duration in milliseconds.
  inline int64_t duration() const { return duration_; }

 private:
  int64_t duration_;
};
}  // namespace foxglove
