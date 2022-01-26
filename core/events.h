#pragma once

#include <string>

namespace foxglove {

enum class PlaybackState {
  kNone,
  kOpening,
  kBuffering,
  kPlaying,
  kPaused,
  kStopped,
  kEnded,
  kError
};

std::string PlaybackStateToString(PlaybackState state);

}  // namespace foxglove