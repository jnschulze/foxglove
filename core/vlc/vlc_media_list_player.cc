#include "vlc/vlc_media_list_player.h"

#include <cassert>
#include <iostream>

namespace foxglove {

VlcMediaListPlayer::VlcMediaListPlayer(
    std::shared_ptr<VlcEnvironment> vlc_environment)
    : environment_(vlc_environment) {}

VlcMediaListPlayer::~VlcMediaListPlayer() {
  player_event_manager_.reset();
  libvlc_media_player_release(media_player_);
}

void VlcMediaListPlayer::SetMediaPlayer(VLC::MediaPlayer& player) {
  auto media_player = player.get();
  libvlc_media_player_retain(player);

  libvlc_media_player_t* old_player;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    old_player = media_player_;
    media_player_ = player;

    // Clone the player's event manager.
    // Destroying the old event manager will unsubscribe from the old player.
    player_event_manager_ =
        std::make_unique<VLC::MediaPlayerEventManager>(player.eventManager());
    SubscribePlayerEvents();
  }

  if (old_player != nullptr) {
    libvlc_media_player_release(old_player);
  }
}

void VlcMediaListPlayer::SubscribePlayerEvents() {
  player_event_manager_->onEndReached([this]() { OnMediaPlayerReachedEnd(); });
}

void VlcMediaListPlayer::OnMediaPlayerReachedEnd() {
  if (ignore_player_events_) {
    return;
  }

  seek_offset_++;
  environment_->task_runner()->Enqueue([weak_this = weak_from_this()]() {
    auto self = weak_this.lock();
    if (self) {
      self->ApplyPendingSeek();
    }
  });
}

void VlcMediaListPlayer::ApplyPendingSeek() {
  std::lock_guard<std::mutex> lock(mutex_);
  PlayItemAtRelativePosition(seek_offset_);
  seek_offset_ = 0;
}

void VlcMediaListPlayer::SetPlaylist(std::unique_ptr<VlcPlaylist> playlist) {
  std::lock_guard<std::mutex> lock(mutex_);
  playlist_ = std::move(playlist);
}

void VlcMediaListPlayer::SetPlaylistMode(PlaylistMode playlist_mode) {
  std::lock_guard<std::mutex> lock(mutex_);
  playlist_mode_ = playlist_mode;
}

void VlcMediaListPlayer::PlayItemAtIndex(int index) {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(HasPlaylist());
  PlayItemAtIndexInternal(index);
}

void VlcMediaListPlayer::Play() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(HasPlaylist());
  ignore_player_events_ = false;
  if (!current_index_.has_value()) {
    PlayItemAtIndexInternal(0);
    return;
  }
  libvlc_media_player_play(media_player_);
}

void VlcMediaListPlayer::Pause() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(IsAttached());
  libvlc_media_player_pause(media_player_);
}

bool VlcMediaListPlayer::StopAsync() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(IsAttached());

  ignore_player_events_ = true;
  bool is_stopping = libvlc_media_player_stop_async(media_player_) == 0;

  current_index_.reset();

  // TODO emit stop event

  return is_stopping;
}

void VlcMediaListPlayer::Next() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(HasPlaylist());
  PlayItemAtRelativePosition(1, true);
}

void VlcMediaListPlayer::Previous() {
  std::lock_guard<std::mutex> lock(mutex_);
  assert(HasPlaylist());
  PlayItemAtRelativePosition(-1, true);
}

void VlcMediaListPlayer::PlayItemAtIndexInternal(int index) {
  auto media_list = vlc_media_list();
  if (!media_list) {
    return;
  }

  libvlc_media_list_lock(media_list);
  int count = libvlc_media_list_count(media_list);
  if (index >= count) {
    libvlc_media_list_unlock(media_list);
    return;
  }

  StartPlayback(index);
  libvlc_media_list_unlock(media_list);
}

void VlcMediaListPlayer::PlayItemAtRelativePosition(int pos, bool manual) {
  auto media_list = vlc_media_list();
  if (!media_list) {
    return;
  }

  libvlc_media_list_lock(media_list);
  auto count = libvlc_media_list_count(media_list);
  if (count == 0) {
    libvlc_media_list_unlock(media_list);
    return;
  }

  auto current_index = current_index_.value_or(0);

  std::optional<int> index;
  if (playlist_mode_ == PlaylistMode::repeat) {
    index = current_index_;
  } else if (playlist_mode_ == PlaylistMode::loop || manual) {
    index = (current_index + pos) % count;
  } else {
    auto i = (current_index + pos);
    if (i >= 0 && i < count) {
      index = i;
    }
  }

  if (!index.has_value()) {
    // TODO: Emit an event that the playlist is done.
    libvlc_media_list_unlock(media_list);
    return;
  }

  StartPlayback(index.value());
  libvlc_media_list_unlock(media_list);
}

void VlcMediaListPlayer::StartPlayback(int index) {
  auto media = libvlc_media_list_item_at_index(vlc_media_list(), index);
  if (!media) {
    return;
  }

  current_index_ = index;

  ignore_player_events_ = true;
  libvlc_media_player_set_media(media_player_, media);
  ignore_player_events_ = false;

  libvlc_media_release(media);
  libvlc_media_player_play(media_player_);
}

}  // namespace foxglove