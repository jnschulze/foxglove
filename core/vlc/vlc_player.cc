#include "vlc/vlc_player.h"

#include <iostream>

#include "vlc/vlc_playlist.h"

namespace foxglove {

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment)
    : environment_(environment) {
  media_player_ = VLC::MediaPlayer(environment_->vlc_instance());
  media_list_player_ = VLC::MediaListPlayer(environment_->vlc_instance());
  media_list_player_.setMediaPlayer(media_player_);

  SetupEventHandlers();
}

VlcPlayer::~VlcPlayer() { std::cerr << "destroying player" << std::endl; }

std::unique_ptr<Playlist> VlcPlayer::CreatePlaylist() {
  return std::make_unique<VlcPlaylist>(environment_->vlc_instance());
}

void VlcPlayer::Open(std::unique_ptr<Media> media) {
  auto playlist = CreatePlaylist();
  playlist->Add(std::move(media));
  Open(std::move(playlist));
}

void VlcPlayer::Open(std::unique_ptr<Playlist> playlist) {
  auto impl_ptr = dynamic_cast<VlcPlaylist*>(playlist.get());
  if (!impl_ptr) {
    return;
  }

  std::unique_ptr<VlcPlaylist> impl(
      static_cast<VlcPlaylist*>(playlist.release()));

  {
    std::lock_guard<std::mutex> lock(op_mutex_);
    OpenInternal(std::move(impl));
  }
}

void VlcPlayer::OpenInternal(std::unique_ptr<VlcPlaylist> playlist) {
  if (playlist_) {
    playlist_->OnUpdate(nullptr);
  }

  StopInternal();

  playlist_ = std::move(playlist);
  playlist_->OnUpdate([this]() { OnPlaylistUpdated(); });

  std::cerr << "LENGTH IS " << playlist_->length() << std::endl;

  LoadPlaylist();
}

void VlcPlayer::OnPlaylistUpdated() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  LoadPlaylist();
}

void VlcPlayer::LoadPlaylist() {
  if (playlist_) {
    media_list_player_.setMediaList(playlist_->vlc_media_list());
  }
}

void VlcPlayer::Play() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    PlayInternal();
  }
}

void VlcPlayer::PlayInternal() { media_list_player_.play(); }

void VlcPlayer::Pause() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    PauseInternal();
  }
}

void VlcPlayer::PauseInternal() { media_list_player_.pause(); }

void VlcPlayer::Stop() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    StopInternal();

    // media_list_player_.pla
  }
}

void VlcPlayer::StopInternal() {
  // media_list_player_.stopAsync();

  // VLC won't stop unless Playback mode is set to default
  media_list_player_.setPlaybackMode(libvlc_playback_mode_default);

  media_player_.stopAsync();
}

void VlcPlayer::Next() {
  SafeInvoke([this]() { media_list_player_.next(); });
}

void VlcPlayer::Previous() {
  SafeInvoke([this]() { media_list_player_.previous(); });
}

void VlcPlayer::SetPlaylistMode(PlaylistMode playlist_mode) {
  libvlc_playback_mode_t mode;
  switch (playlist_mode) {
    case PlaylistMode::loop:
      mode = libvlc_playback_mode_loop;
      break;
    case PlaylistMode::repeat:
      mode = libvlc_playback_mode_repeat;
      break;
    default:
      mode = libvlc_playback_mode_default;
  }

  std::cerr << "SET playback mode to " << mode << std::endl;

  SafeInvoke([this, mode]() { media_list_player_.setPlaybackMode(mode); });
}

void VlcPlayer::SafeInvoke(VoidCallback callback) {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    callback();
  }
}

void VlcPlayer::SetupEventHandlers() {
  auto& event_manager = media_player_.eventManager();

  Subscribe(event_manager.onVout(
      [](int x) { std::cerr << "on vout changed " << x << std::endl; }));

  Subscribe(event_manager.onNothingSpecial(
      [this] { HandleVlcState(PlaybackState::kNone); }));

  Subscribe(event_manager.onOpening(
      [this] { HandleVlcState(PlaybackState::kOpening); }));

  Subscribe(event_manager.onPlaying(
      [this] { HandleVlcState(PlaybackState::kPlaying); }));

  Subscribe(event_manager.onPaused(
      [this] { HandleVlcState(PlaybackState::kPaused); }));

  Subscribe(event_manager.onStopped([this] {
    std::cerr << "VLC PLAYBACK STOPPED" << std::endl;

    HandleVlcState(PlaybackState::kStopped);
  }));

  Subscribe(event_manager.onEndReached(
      [this] { HandleVlcState(PlaybackState::kEnded); }));
}

void VlcPlayer::HandleVlcState(PlaybackState state) {
  std::cerr << "PLAYBACK STATE CHANGED " << PlaybackStateToString(state)
            << std::endl;
}

void VlcPlayer::Subscribe(VLC::EventManager::RegisteredEvent ev) {
  event_registrations_.push_back(std::make_unique<VlcEventRegistration>(ev));
}

}  // namespace foxglove