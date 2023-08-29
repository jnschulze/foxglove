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

#ifdef NDEBUG
#define PLAYER_LOG(msg)
#else
#define PLAYER_LOG(msg) \
  std::cerr << "Player [" << id() << "]: " << msg << std::endl;
#endif

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment)
    : environment_(environment) {
  media_player_ = VLC::MediaPlayer(*environment_->vlc_instance());

  media_list_player_ = std::make_shared<VlcMediaListPlayer>(environment);
  media_list_player_->SetMediaPlayer(media_player_);

  SetupEventHandlers();
}

VlcPlayer::~VlcPlayer() { Shutdown(); }

void VlcPlayer::Shutdown() {
  PLAYER_LOG("Shutting down");

  if (shutting_down_) {
    return;
  }
  shutting_down_ = true;

  // Unsubscribe from all events.
  player_event_manager_.reset();

  auto playlist = media_list_player_->playlist();
  if (playlist) {
    playlist->OnUpdate(nullptr);
  }

  media_list_player_->SetPlaylist(nullptr);
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
    winrt::com_ptr<IDXGIAdapter> adapter) const {
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
  return std::make_unique<VlcPlaylist>();
}

void VlcPlayer::Open(std::unique_ptr<Media> media) {
  auto playlist = std::make_unique<VlcPlaylist>();
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
  if (!IsValid()) {
    return;
  }

  auto old_playlist = media_list_player_->playlist();
  if (old_playlist) {
    old_playlist->OnUpdate(nullptr);
  }

  // StopSyncInternal();
  StopInternal();
  // media_state_.Reset();
  state_.is_playlist = is_playlist;

  playlist->OnUpdate([this]() { OnPlaylistUpdated(); });
  media_list_player_->SetPlaylist(std::move(playlist));
}

void VlcPlayer::OnPlaylistUpdated() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  // LoadPlaylist();
}

void VlcPlayer::Play() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    PlayInternal();
  }
}

void VlcPlayer::PlayInternal() { media_list_player_->Play(); }

void VlcPlayer::Pause() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    PauseInternal();
  }
}

void VlcPlayer::PauseInternal() { media_list_player_->Pause(); }

void VlcPlayer::Stop() {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    StopInternal();
  }
}

bool VlcPlayer::StopInternal() {
  /*
  std::promise<bool> promise;
  std::async([this, &promise]() {
    promise.set_value(media_list_player_->StopAsync());
  }).wait();
  return promise.get_future().get();
  */

  return media_list_player_->StopAsync();
}

void VlcPlayer::StopSync(std::optional<std::chrono::milliseconds> timeout) {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    StopSyncInternal(timeout);
  }
}

void VlcPlayer::StopSyncInternal(
    std::optional<std::chrono::milliseconds> timeout) {
  PLAYER_LOG("Pre StopSync");

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

  PLAYER_LOG("Post StopSync");
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
      case PlaybackState::kPaused: {
        media_player_.setTime(time, false);
        // VLC doesn't update it's position when paused.
        // So just update the state's position directly.
        auto length = duration();
        if (length != 0) {
          media_state_.position = time / static_cast<double>(length);
          NotifyPositionChanged();
        }
      } break;
      case PlaybackState::kNone:
      case PlaybackState::kOpening:
      case PlaybackState::kBuffering:
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
  SafeInvoke([this]() { media_list_player_->Next(); });
}

void VlcPlayer::Previous() {
  SafeInvoke([this]() { media_list_player_->Previous(); });
}

void VlcPlayer::SetPlaylistMode(PlaylistMode mode) {
  SafeInvoke([this, mode]() {
    state_.playlist_mode = mode;
    SetPlaylistModeInternal(mode);
  });
}

void VlcPlayer::SetPlaylistModeInternal(PlaylistMode playlist_mode) {
  media_list_player_->SetPlaylistMode(playlist_mode);
}

void VlcPlayer::SetVolume(double volume) {
  SafeInvoke([this, volume]() {
    media_player_.setVolume(static_cast<int32_t>(volume * 100));
  });
}

void VlcPlayer::SetMute(bool flag) {
  SafeInvoke([this, flag]() { media_player_.setMute(flag); });
}

int64_t VlcPlayer::duration() { return media_state_.duration.value_or(0); }

void VlcPlayer::SafeInvoke(VoidCallback callback) {
  std::lock_guard<std::mutex> lock(op_mutex_);
  if (IsValid()) {
    callback();
  }
}

void VlcPlayer::SetupEventHandlers() {
  player_event_manager_ = std::make_unique<VLC::MediaPlayerEventManager>(
      media_player_.eventManager());

  player_event_manager_->onEncounteredError(
      []() { std::cerr << "Encountered error" << std::endl; });

  player_event_manager_->onNothingSpecial(
      [this] { HandleVlcState(PlaybackState::kNone); });

  player_event_manager_->onOpening(
      [this] { HandleVlcState(PlaybackState::kOpening); });

  player_event_manager_->onPlaying(
      [this] { HandleVlcState(PlaybackState::kPlaying); });

  player_event_manager_->onPaused(
      [this] { HandleVlcState(PlaybackState::kPaused); });

  player_event_manager_->onStopping(
      [this] { HandleVlcState(PlaybackState::kEnded); });

  player_event_manager_->onStopped(
      [this] { HandleVlcState(PlaybackState::kStopped); });

  player_event_manager_->onMediaChanged(
      [this](VLC::MediaPtr media) { HandleMediaChanged(std::move(media)); });

  player_event_manager_->onLengthChanged(
      [this](int64_t length) { HandleLengthChanged(length); });
  // player_event_manager_->onLengthChanged([this](int64_t length) {
  //   // std::cerr << "length changed " << length << std::endl;
  // });

  player_event_manager_->onPositionChanged(
      [this](double position) { HandlePositionChanged(position); });

  player_event_manager_->onSeekableChanged(
      [this](bool is_seekable) { HandleSeekableChanged(is_seekable); });

  player_event_manager_->onAudioVolume(
      [this](float value) { HandleVolumeChanged(value); });

  player_event_manager_->onMuted([this]() { HandleMuteChanged(true); });
  player_event_manager_->onUnmuted([this]() { HandleMuteChanged(false); });
}

void VlcPlayer::HandleVlcState(PlaybackState state) {
  bool has_change = false;

  PLAYER_LOG("STATE IS " << PlaybackStateToString(state))

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
          media_state_.position = 0;
          std::cerr << "Video stopped" << std::endl;
          break;
        case PlaybackState::kEnded:
          media_state_.position = 0;
          break;
        default:
          break;
      }
      has_change = true;
    }
  }

  if (!IsValid()) {
    PLAYER_LOG("Instance down");
    return;
  }

  if (has_change) {
    if (state == PlaybackState::kPlaying) {
      OnPlay();
    }
    NotifyStateChanged();
    NotifyPositionChanged();
  }
}

void VlcPlayer::OnPlay() {
  // Ensure media_state_ is in sync with the actual state.
  HandleMuteChanged(media_player_.mute());
  HandleVolumeChanged(media_player_.volume() / 100.0f);
}

void VlcPlayer::HandleMuteChanged(bool is_mute) {
  if (media_state_.is_mute != is_mute) {
    media_state_.is_mute = is_mute;
    if (event_delegate_) {
      event_delegate_->OnMute(media_state_.is_mute);
    }
  }
}

void VlcPlayer::HandleVolumeChanged(float volume) {
  volume = std::clamp(volume, 0.0f, 1.0f);
  if (volume != media_state_.volume) {
    media_state_.volume = volume;
    if (event_delegate_) {
      event_delegate_->OnVolumeChanged(volume);
    }
  }
}

void VlcPlayer::HandleMediaChanged(VLC::MediaPtr vlc_media) {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (!IsValid()) {
      return;
    }

    auto playlist = media_list_player_->playlist();
    auto index = playlist->vlc_media_list().indexOfItem(*vlc_media.get());
    if (index == -1) {
      return;
    }

    media_state_.index = index;
    // media_state_.current_item = vlc_media;

    media_state_.duration.reset();
    media_state_.position = 0;
    auto duration = vlc_media->duration();
    if (duration != -1) {
      media_state_.duration = duration;
    }
  }
  NotifyPositionChanged();
}

void VlcPlayer::HandleLengthChanged(int64_t length) {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    media_state_.duration = length;
  }
  NotifyMediaChanged();
  NotifyPositionChanged();
}

void VlcPlayer::HandlePositionChanged(double position) {
  position = std::clamp(position, 0.0, 1.0);
  media_state_.position = position;
  NotifyPositionChanged();
}

void VlcPlayer::HandleSeekableChanged(bool is_seekable) {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    media_state_.is_seekable = is_seekable;
  }
  NotifyStateChanged();
}

void VlcPlayer::NotifyStateChanged() {
  if (event_delegate_) {
    event_delegate_->OnPlaybackStateChanged(media_state_.playback_state,
                                            media_state_.is_seekable);
  }
}

void VlcPlayer::NotifyPositionChanged() {
  if (event_delegate_) {
    event_delegate_->OnPositionChanged(media_state_.position, duration());
  }
}

void VlcPlayer::NotifyMediaChanged() {
  auto playlist = media_list_player_->playlist();
  if (playlist && event_delegate_) {
    auto media = playlist->GetItem(media_state_.index);
    if (media) {
      auto media_info = std::make_unique<MediaInfo>(duration());
      event_delegate_->OnMediaChanged(media.get(), std::move(media_info),
                                      media_state_.index);
    }
  }
}

}  // namespace foxglove