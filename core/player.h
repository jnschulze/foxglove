#pragma once

#include <memory>

#include "base/error_details.h"
#include "base/status.h"
#include "events.h"
#include "media/media.h"
#include "media/media_info.h"
#include "video/video_output_factory.h"

namespace foxglove {

enum LoopMode { kOff, kLoop, kLastValue };

struct MediaPlaybackPosition {
  double position = 0;
  int64_t duration = 0;
};

class PlayerEventDelegate {
 public:
  virtual ~PlayerEventDelegate() = default;

  virtual void OnMediaChanged(std::unique_ptr<Media> media) {}
  virtual void OnPlaybackStateChanged(PlaybackState playback_state) {}
  virtual void OnIsSeekableChanged(bool is_seekable) {}
  virtual void OnPositionChanged(const MediaPlaybackPosition& position) {}
  virtual void OnRateChanged(double rate) {}
  virtual void OnVolumeChanged(double volume) {}
  virtual void OnMute(bool is_muted) {}
  virtual void OnVideoDimensionsChanged(int32_t width, int32_t height) {}
};

template <typename TVideoOutput>
class Player : public VideoOutputFactory<TVideoOutput> {
 public:
  typedef TVideoOutput VideoOutputType;

  virtual ~Player() = default;

  virtual void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) = 0;
  virtual PlayerEventDelegate* event_delegate() const = 0;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }

  virtual Status<ErrorDetails> SetVideoOutput(
      std::unique_ptr<VideoOutputType> output) = 0;
  virtual VideoOutputType* GetVideoOutput() const = 0;

  virtual bool Open(std::unique_ptr<Media> media) = 0;
  virtual bool Play() = 0;
  virtual void Pause() = 0;
  virtual bool Stop() = 0;
  virtual void SeekPosition(double position) = 0;
  virtual void SeekTime(int64_t time) = 0;
  virtual void SetRate(float rate) = 0;
  virtual void SetLoopMode(LoopMode loop_mode) = 0;
  virtual void SetVolume(double volume) = 0;
  virtual void SetMute(bool muted) = 0;
  virtual int64_t duration() = 0;
};

}  // namespace foxglove
