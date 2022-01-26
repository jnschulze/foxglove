#pragma once

#include <iostream>
#include <mutex>

#include "events.h"
#include "player.h"
#include "vlc/vlc_environment.h"
#include "vlc/vlc_playlist.h"

namespace foxglove {

struct VlcEventRegistration {
 public:
  VlcEventRegistration(VLC::EventManager::RegisteredEvent registration)
      : registration_(registration) {}

  ~VlcEventRegistration() {
    std::cerr << "Destroying event registration" << std::endl;
    registration_->unregister();
  }

  VLC::EventManager::RegisteredEvent registration_;
};

class VlcPlayer : public Player {
 public:
  VlcPlayer(std::shared_ptr<VlcEnvironment> environment);
  ~VlcPlayer() override;

  inline bool IsValid() { return true; }

  void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) override {
    event_delegate_ = std::move(event_delegate);
  }

  std::unique_ptr<Playlist> CreatePlaylist() override;

  void Open(std::unique_ptr<Media> media) override;
  void Open(std::unique_ptr<Playlist> playlist) override;

  void Play() override;
  void Pause() override;
  void Stop() override;

  void Next() override;
  void Previous() override;

  void SetPlaylistMode(PlaylistMode playlist_mode) override;

 private:
  typedef std::function<void()> VoidCallback;
  std::mutex op_mutex_;
  std::shared_ptr<VlcEnvironment> environment_;
  std::unique_ptr<PlayerEventDelegate> event_delegate_;
  VLC::MediaPlayer media_player_;
  VLC::MediaListPlayer media_list_player_;
  std::unique_ptr<VlcPlaylist> playlist_;
  std::vector<std::unique_ptr<VlcEventRegistration>> event_registrations_;

  void OpenInternal(std::unique_ptr<VlcPlaylist> playlist);
  void PlayInternal();
  void PauseInternal();
  void StopInternal();
  void OnPlaylistUpdated();
  void LoadPlaylist();

  void SafeInvoke(VoidCallback callback);

  void SetupEventHandlers();
  void HandleVlcState(PlaybackState state);
  void Subscribe(VLC::EventManager::RegisteredEvent ev);
};

}  // namespace foxglove