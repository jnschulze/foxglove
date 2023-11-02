#pragma once

#include <memory>

#include "events.h"
#include "media/media.h"
#include "media/media_info.h"
#include "media/playlist.h"
#include "video/video_output_factory.h"

namespace foxglove {

class PlayerEventDelegate {
 public:
  virtual ~PlayerEventDelegate() = default;

  virtual void OnMediaChanged(const Media* media,
                              std::unique_ptr<MediaInfo> media_info,
                              size_t index) {}
  virtual void OnPlaybackStateChanged(PlaybackState playback_state,
                                      bool is_seekable) {}
  virtual void OnPositionChanged(double position, int64_t duration) {}
  virtual void OnRateChanged(double rate) {}
  virtual void OnVolumeChanged(double volume) {}
  virtual void OnMute(bool is_muted) {}
  virtual void OnVideoDimensionsChanged(int32_t width, int32_t height) {}
  virtual void OnError(int32_t error_code, std::string error_message) {}
};

class Player : public VideoOutputFactory {
 public:
  virtual ~Player() = default;

  virtual void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) = 0;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }

  virtual void SetVideoOutput(std::unique_ptr<VideoOutput> output) = 0;
  virtual VideoOutput* GetVideoOutput() const = 0;

  virtual std::unique_ptr<Playlist> CreatePlaylist() = 0;

  virtual bool Open(std::unique_ptr<Media> media) = 0;
  virtual bool Open(std::unique_ptr<Playlist> playlist) = 0;
  virtual bool Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;
  virtual void SeekPosition(float position) = 0;
  virtual void SeekTime(int64_t time) = 0;
  virtual bool Next() = 0;
  virtual bool Previous() = 0;
  virtual void SetRate(float rate) = 0;
  virtual void SetPlaylistMode(PlaylistMode playlist_mode) = 0;
  virtual void SetVolume(double volume) = 0;
  virtual void SetMute(bool muted) = 0;

  virtual int64_t duration() = 0;
};

}  // namespace foxglove