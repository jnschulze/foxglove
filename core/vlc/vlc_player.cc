#include "vlc/vlc_player.h"

#include <iostream>

#include "vlc/vlc_d3d11_output.h"
#include "vlc/vlc_pixel_buffer_output.h"
#include "vlc/vlc_playlist.h"

namespace foxglove {

namespace {

template <typename TDerived, typename TBase>
std::unique_ptr<TDerived> unique_pointer_cast(TBase base) {
  auto impl_ptr = dynamic_cast<TDerived*>(base.get());
  if (!impl_ptr) {
    return nullptr;
  }
  return std::unique_ptr<TDerived>(static_cast<TDerived*>(base.release()));
}

}  // namespace

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment)
    : environment_(environment) {
  media_player_ = VLC::MediaPlayer(environment_->vlc_instance());
  media_list_player_ = VLC::MediaListPlayer(environment_->vlc_instance());
  media_list_player_.setMediaPlayer(media_player_);

  SetupEventHandlers();
}

VlcPlayer::~VlcPlayer() { Shutdown(); }

void VlcPlayer::Shutdown() {
  if (shutting_down_) {
    return;
  }
  shutting_down_ = true;

  video_output_->Shutdown();

  if (playlist_) {
    playlist_->OnUpdate(nullptr);
    playlist_.reset();
  }

  StopSync();
}

std::unique_ptr<VideoOutput> VlcPlayer::CreatePixelBufferOutput(
    std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
    PixelFormat pixel_format) const {
  return std::make_unique<VlcPixelBufferOutput>(std::move(output_delegate),
                                                pixel_format);
}

#ifdef _WIN32
std::unique_ptr<VideoOutput> VlcPlayer::CreateD3D11Output(
    std::unique_ptr<D3D11OutputDelegate> output_delegate,
    IDXGIAdapter* adapter) const {
  return std::make_unique<VlcD3D11Output>(std::move(output_delegate), adapter);
}
#endif

void VlcPlayer::SetVideoOutput(std::unique_ptr<VideoOutput> video_output) {
  if (auto vlc_output =
          unique_pointer_cast<VlcVideoOutput>(std::move(video_output))) {
    video_output_ = std::move(vlc_output);
    video_output_->OnDimensionsChanged(
        [this](const VideoDimensions& dimensions) {
          if (event_delegate_) {
            event_delegate_->OnVideoDimensionsChanged(dimensions.width,
                                                      dimensions.height);
          }
        });
    video_output_->Attach(this);
  }
}

std::unique_ptr<Playlist> VlcPlayer::CreatePlaylist() {
  return std::make_unique<VlcPlaylist>(environment_->vlc_instance());
}

void VlcPlayer::Open(std::unique_ptr<Media> media) {
  auto playlist = std::make_unique<VlcPlaylist>(environment_->vlc_instance());
  playlist->Add(std::move(media));

  {
    std::lock_guard<std::mutex> lock(op_mutex_);
    OpenInternal(std::move(playlist), false);
  }
}

void VlcPlayer::Open(std::unique_ptr<Playlist> playlist) {
  if (auto vlc_playlist =
          unique_pointer_cast<VlcPlaylist>(std::move(playlist))) {
    std::lock_guard<std::mutex> lock(op_mutex_);
    OpenInternal(std::move(vlc_playlist), true);
  }
}

void VlcPlayer::OpenInternal(std::unique_ptr<VlcPlaylist> playlist,
                             bool is_playlist) {
  if (playlist_) {
    playlist_->OnUpdate(nullptr);
  }

  media_list_player_.setMediaList(VLC::MediaList());

  StopSyncInternal();
  media_state_.Reset();
  state_.is_playlist = is_playlist;

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

void VlcPlayer::PlayInternal() {
  std::cerr << "PlayInternal " << std::endl;

  media_list_player_.play();
}

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
  }
}

bool VlcPlayer::StopInternal() {
  auto playlist_mode = state_.playlist_mode;

  // VLC won't stop unless Playback mode is set to default
  // SetPlaylistModeInternal(PlaylistMode::single);

  bool stopping = libvlc_media_player_stop_async(media_player_.get()) == 0;
  libvlc_media_list_player_stop_async(media_list_player_.get());

  // Restore previous playlist mode
  // SetPlaylistModeInternal(playlist_mode);

  return stopping;
  // return false;
}

void VlcPlayer::StopSync(std::optional<std::chrono::milliseconds> timeout) {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    StopSyncInternal(timeout);
  }
}

void VlcPlayer::StopSyncInternal(
    std::optional<std::chrono::milliseconds> timeout) {
  std::cerr << "STOP SYNC" << std::endl;

  {
    std::unique_lock<std::mutex> lock(stop_mutex_);
    is_stopped_ = false;

    bool is_stopping = StopInternal();
    if (is_stopping) {
      if (timeout.has_value()) {
        stop_cond_.wait_for(lock, timeout.value(),
                            [this]() { return is_stopped_; });
      } else {
        stop_cond_.wait(lock, [this]() { return is_stopped_; });
      }
    } else {
      is_stopped_ = true;
    }
  }

  std::cerr << "STOPPED" << std::endl;
}

void VlcPlayer::SeekPosition(float position) {
  auto media_duration = duration();
  auto time = static_cast<int64_t>(media_duration * position);
  SeekTime(time);
}

void VlcPlayer::SeekTime(int64_t time) {
  SafeInvoke([this, time]() {
    auto state = media_state_.playback_state;
    switch (state) {
      case PlaybackState::kPlaying:
        media_player_.setTime(time, false);
      case PlaybackState::kNone:
      case PlaybackState::kOpening:
      case PlaybackState::kBuffering:
      case PlaybackState::kPaused:
      case PlaybackState::kStopped:
      case PlaybackState::kEnded:
        media_state_.pending_seek_time = time;
      default:
        break;
    }
  });
}

void VlcPlayer::SetRate(float rate) {
  SafeInvoke([this, rate]() { media_player_.setRate(rate); });
}

void VlcPlayer::Next() {
  SafeInvoke([this]() { media_list_player_.next(); });
}

void VlcPlayer::Previous() {
  SafeInvoke([this]() { media_list_player_.previous(); });
}

void VlcPlayer::SetPlaylistMode(PlaylistMode mode) {
  SafeInvoke([this, mode]() {
    state_.playlist_mode = mode;
    SetPlaylistModeInternal(mode);
  });
}

void VlcPlayer::SetPlaylistModeInternal(PlaylistMode playlist_mode) {
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
  media_list_player_.setPlaybackMode(mode);
}

void VlcPlayer::SetVolume(double volume) {
  SafeInvoke([this, volume]() {
    media_player_.setVolume(static_cast<int32_t>(volume * 100));
  });
}

void VlcPlayer::SetMute(bool flag) {
  SafeInvoke([this, flag]() { media_player_.setMute(flag); });
}

int64_t VlcPlayer::duration() {
  // return std::max<int64_t>(media_player_.length(), 0);
  return media_state_.duration;
}

void VlcPlayer::SafeInvoke(VoidCallback callback) {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    callback();
  }
}

void VlcPlayer::SetupEventHandlers() {
  auto& event_manager = media_player_.eventManager();

  // media_list_player_.eventManager().onNextItemSet

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

  Subscribe(event_manager.onMediaChanged(
      [this](VLC::MediaPtr media) { HandleMediaChanged(std::move(media)); }));

  Subscribe(event_manager.onLengthChanged([this](int64_t length) {
    std::cerr << "length changed " << length << std::endl;
  }));

  Subscribe(event_manager.onPositionChanged(
      [this](float position) { HandlePositionChanged(position); }));

  Subscribe(event_manager.onSeekableChanged(
      [this](bool is_seekable) { HandleSeekableChanged(is_seekable); }));

  Subscribe(event_manager.onAudioVolume([this](float value) {
    if (event_delegate_) {
      event_delegate_->OnVolumeChanged(value);
    }
  }));

  Subscribe(event_manager.onMuted([this]() {
    if (event_delegate_) {
      event_delegate_->OnMute(true);
    }
  }));

  Subscribe(event_manager.onUnmuted([this]() {
    if (event_delegate_) {
      event_delegate_->OnMute(false);
    }
  }));
}

void VlcPlayer::HandleVlcState(PlaybackState state) {
  bool has_change = false;

  if (state == PlaybackState::kStopped) {
    {
      std::lock_guard<std::mutex> lock(stop_mutex_);
      is_stopped_ = true;
    }
    stop_cond_.notify_one();
  }

  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto pending_seek_time = media_state_.pending_seek_time.value_or(-1);
    if (state == PlaybackState::kPlaying && pending_seek_time != -1) {
      media_state_.pending_seek_time.reset();
      // This must be called from another thread to avoid deadlocks.
      environment_->task_runner()->Enqueue(
          [player = media_player_, pending_seek_time]() mutable {
            player.setTime(pending_seek_time, false);
          });
    }

    if (media_state_.playback_state != state) {
      media_state_.playback_state = state;
      switch (state) {
        case PlaybackState::kStopped:
          // case PlaybackState::kEnded:
          std::cerr << "Video stopped" << std::endl;
          // state_.Reset();
          break;
        default:
          break;
      }
      has_change = true;
    }
  }

  if (!IsValid()) {
    std::cerr << "INSTANCE DOWN. NOT FIRING CB" << std::endl;
    return;
  }

  if (has_change) {
    NotifyStateChanged();
  }
}

void VlcPlayer::HandleMediaChanged(VLC::MediaPtr vlc_media) {
  std::cerr << "MEDIA CHANGED" << std::endl;

  std::lock_guard<std::mutex> lock(state_mutex_);

  if (!IsValid() || !playlist_) {
    std::cerr << "IGNORING MEDIA CHANGE" << std::endl;
    return;
  }

  auto index = playlist_->vlc_media_list().indexOfItem(*vlc_media.get());
  if (index == -1) {
    return;
  }

  media_state_.index = index;
  media_state_.current_item = vlc_media;
  media_state_.duration = vlc_media->duration();

  if (event_delegate_) {
    auto media = playlist_->GetItem(media_state_.index);
    if (media) {
      auto media_info = std::make_unique<MediaInfo>(vlc_media->duration());
      event_delegate_->OnMediaChanged(media.get(), std::move(media_info),
                                      index);
    }
  }
}

void VlcPlayer::HandlePositionChanged(float position) {
  double pos = std::clamp(static_cast<double>(position), 0.0, 1.0);
  if (event_delegate_) {
    event_delegate_->OnPositionChanged(pos, duration());
  }
}

void VlcPlayer::HandleSeekableChanged(bool is_seekable) {
  media_state_.is_seekable = is_seekable;
  NotifyStateChanged();
}

void VlcPlayer::NotifyStateChanged() {
  if (event_delegate_) {
    event_delegate_->OnPlaybackStateChanged(media_state_.playback_state,
                                            media_state_.is_seekable);
  }
}

void VlcPlayer::Subscribe(VLC::EventManager::RegisteredEvent ev) {
  event_registrations_.push_back(std::make_unique<VlcEventRegistration>(ev));
}

}  // namespace foxglove