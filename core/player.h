#pragma once

#include <memory>

#include "media/media.h"
#include "media/playlist.h"

namespace foxglove {

class PlayerEventDelegate {
 public:
  virtual ~PlayerEventDelegate() = default;
};

class Player {
 public:
  virtual ~Player() = default;

  virtual void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) = 0;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }

  virtual std::unique_ptr<Playlist> CreatePlaylist() = 0;

  virtual void Open(std::unique_ptr<Media> media) = 0;
  virtual void Open(std::unique_ptr<Playlist> playlist) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Next() = 0;
  virtual void Previous() = 0;
  virtual void SetPlaylistMode(PlaylistMode playlist_mode) = 0;
};

}  // namespace foxglove