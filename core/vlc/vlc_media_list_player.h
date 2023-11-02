#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vlcpp/vlc.hpp>

#include "vlc/vlc_environment.h"
#include "vlc/vlc_playlist.h"

namespace foxglove {

class VlcMediaListPlayer
    : public std::enable_shared_from_this<VlcMediaListPlayer> {
 public:
  VlcMediaListPlayer(std::shared_ptr<VlcEnvironment> vlc_environment);
  ~VlcMediaListPlayer();

  inline bool IsAttached() const { return media_player_ != nullptr; }
  inline bool HasPlaylist() const {
    return IsAttached() && playlist_ != nullptr;
  }

  void SetMediaPlayer(VLC::MediaPlayer& player);
  void SetPlaylist(std::unique_ptr<VlcPlaylist> playlist);
  void SetPlaylistMode(PlaylistMode playlist_mode);
  bool PlayItemAtIndex(int index);
  bool Play();
  void Pause();
  bool StopAsync();
  bool Next();
  bool Previous();

  VlcPlaylist* playlist() const { return playlist_.get(); }

 private:
  std::shared_ptr<VlcEnvironment> environment_;
  std::mutex mutex_;
  std::unique_ptr<VlcPlaylist> playlist_;
  std::unique_ptr<VLC::MediaPlayerEventManager> player_event_manager_;
  libvlc_media_player_t* media_player_ = nullptr;
  PlaylistMode playlist_mode_ = PlaylistMode::single;
  std::optional<int> current_index_;
  int seek_offset_ = 0;
  std::atomic<bool> ignore_player_events_ = false;

  void SubscribePlayerEvents();
  void OnMediaPlayerStopping();
  bool PlayItemAtIndexInternal(int index);
  bool PlayItemAtRelativePosition(int pos, bool manual = false);
  bool StartPlayback(int index);
  void ApplyPendingSeek();

  inline libvlc_media_list_t* vlc_media_list() {
    return playlist_ != nullptr ? playlist_->vlc_media_list().get() : nullptr;
  }
};

}  // namespace foxglove