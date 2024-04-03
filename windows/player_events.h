#pragma once

#include <cstdint>

namespace foxglove {
namespace windows {

constexpr const char kEventType[] = "type";
constexpr const char kEventValue[] = "value";

enum Events : int32_t {
  kNone,
  kInitialized,
  kPositionChanged,
  kPlaybackStateChanged,
  kMediaChanged,
  kRateChanged,
  kVolumeChanged,
  kMuteChanged,
  kVideoDimensionsChanged,
  kIsSeekableChanged
};

}  // namespace windows
}  // namespace foxglove
