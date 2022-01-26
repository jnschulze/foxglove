#include "events.h"

namespace foxglove {

std::string PlaybackStateToString(PlaybackState state) {
  switch (state) {
    case PlaybackState::kNone:
      return "none";
    case PlaybackState::kOpening:
      return "opening";
    case PlaybackState::kBuffering:
      return "buffering";
    case PlaybackState::kPlaying:
      return "playing";
    case PlaybackState::kPaused:
      return "paused";
    case PlaybackState::kStopped:
      return "stopped";
    case PlaybackState::kEnded:
      return "ended";
    case PlaybackState::kError:
      return "error";
    default:
      return "unknown";
  }
}

}