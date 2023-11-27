#pragma once

#include <iostream>
#include <memory>
#include <mutex>

#include "base/thread_checker.h"
#include "events.h"
#include "player.h"
#include "vlc/vlc_environment.h"
#include "vlc/vlc_media.h"
#include "vlc/vlc_video_output.h"
#include "vlc_player.h"
#include "vlcpp/vlc.hpp"

namespace foxglove {

#ifdef NDEBUG
#define PLAYER_LOG(msg)
#else
#define PLAYER_LOG(msg) \
  std::cerr << "Player [" << id() << "]: " << msg << std::endl;
#endif

struct VlcMediaState {
  std::unique_ptr<VlcMedia> media;
  PlaybackState playback_state;
  std::optional<int64_t> duration;
  double position;
  bool is_seekable;
  bool is_mute;
  float volume;
  bool has_error;

  VlcMediaState() { Reset(); }

  void Reset() {
    has_error = false;
    media.reset();
    duration.reset();
    position = 0;
    playback_state = PlaybackState::kNone;
    is_seekable = false;
    is_mute = false;
    volume = 0;
  }

  bool CanSeek() {
    switch (playback_state) {
      case PlaybackState::kPlaying:
      case PlaybackState::kPaused:
        return true;
      default:
        return false;
    }
  }

  MediaPlaybackPosition GetPosition() {
    return {position, duration.value_or(0)};
  }
};

struct VlcPlayerState {
  LoopMode loop_mode;
  bool stopped_by_user;

  VlcPlayerState() { Reset(); }

  void Reset() {
    loop_mode = LoopMode::kOff;
    stopped_by_user = false;
  }
};

class VlcPlayer::Impl : public std::enable_shared_from_this<VlcPlayer::Impl> {
 public:
  Impl(std::shared_ptr<VlcEnvironment> env, int64_t id)
      : environment_(env), id_(id) {
    media_player_ = VLC::MediaPlayer(*environment_->vlc_instance());
    SetupEventHandlers();
  }

  ~Impl() { assert(thread_checker_.IsCreationThreadCurrent()); }

  void SetEventDelegate(std::unique_ptr<PlayerEventDelegate> event_delegate) {
    assert(thread_checker_.IsCreationThreadCurrent());
    event_delegate_ = std::move(event_delegate);
  }

  PlayerEventDelegate* event_delegate() const {
    assert(thread_checker_.IsCreationThreadCurrent());
    return event_delegate_.get();
  }

  void SetVideoOutput(std::unique_ptr<VlcVideoOutput> video_output) {
    assert(thread_checker_.IsCreationThreadCurrent());
    video_output_ = std::move(video_output);
    video_output_->OnDimensionsChanged(
        [this](const VideoDimensions& dimensions) {
          if (event_delegate_) {
            event_delegate_->OnVideoDimensionsChanged(dimensions.width,
                                                      dimensions.height);
          }
        });
    video_output_->Attach(media_player_.get());
  }

  VlcVideoOutput* GetVideoOutput() const {
    assert(thread_checker_.IsCreationThreadCurrent());
    return video_output_.get();
  }

  bool Open(std::unique_ptr<Media> media) {
    assert(thread_checker_.IsCreationThreadCurrent());
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      media_state_.media =
          media ? std::make_unique<VlcMedia>(std::move(media)) : nullptr;
      media_state_.has_error = false;
    }
    Stop();

    auto vlc_media =
        media_state_.media ? media_state_.media->vlc_media() : nullptr;
    libvlc_media_player_set_media(media_player_.get(), vlc_media);

    return true;
  }

  bool Play() {
    assert(thread_checker_.IsCreationThreadCurrent());
    return media_player_.play();
  }

  bool Stop() {
    assert(thread_checker_.IsCreationThreadCurrent());
    return libvlc_media_player_stop_async(media_player_.get()) == 0;
  }
  void Pause() {
    assert(thread_checker_.IsCreationThreadCurrent());
    media_player_.pause();
  }

  void SeekPosition(double position) {
    assert(thread_checker_.IsCreationThreadCurrent());
    bool did_update_position;
    MediaPlaybackPosition playback_position;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      auto media_duration = media_state_.duration.value_or(0);
      auto time = static_cast<int64_t>(media_duration * position);
      did_update_position = SeekTimeLocked(time);
      if (did_update_position) {
        playback_position = media_state_.GetPosition();
      }
    }
    if (did_update_position) {
      NotifyPositionChanged(playback_position);
    }
  }

  void SeekTime(int64_t time) {
    assert(thread_checker_.IsCreationThreadCurrent());
    bool did_update_position;
    MediaPlaybackPosition playback_position;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      did_update_position = SeekTimeLocked(time);
      if (did_update_position) {
        playback_position = media_state_.GetPosition();
      }
    }
    if (did_update_position) {
      NotifyPositionChanged(playback_position);
    }
  }

  bool SeekTimeLocked(int64_t time) {
    if (media_state_.CanSeek()) {
      media_player_.setTime(time, false);
      if (media_state_.playback_state == PlaybackState::kPaused) {
        // VLC doesn't update it's position when paused.
        // So just update the state's position directly.
        auto duration = media_state_.duration;
        if (duration.has_value()) {
          auto new_position = time / static_cast<double>(duration.value());
          if (new_position != media_state_.position) {
            media_state_.position = new_position;
            return true;
          }
        }
      }
    }
    return false;
  }

  void SetRate(float rate) {
    assert(thread_checker_.IsCreationThreadCurrent());
    media_player_.setRate(rate);
  }

  void SetLoopMode(LoopMode mode) {
    assert(thread_checker_.IsCreationThreadCurrent());
    state_.loop_mode = mode;
  }

  void SetVolume(double volume) {
    assert(thread_checker_.IsCreationThreadCurrent());
    media_player_.setVolume(static_cast<int32_t>(volume * 100));
  }
  void SetMute(bool flag) {
    assert(thread_checker_.IsCreationThreadCurrent());
    media_player_.setMute(flag);
  }

  int64_t id() const { return id_; }

  int64_t duration() const {
    assert(thread_checker_.IsCreationThreadCurrent());
    return media_state_.duration.value_or(0);
  }

 private:
  ThreadChecker thread_checker_;
  int64_t id_;
  VlcMediaState media_state_;
  VlcPlayerState state_;
  std::mutex state_mutex_;
  bool shutting_down_ = false;
  std::shared_ptr<VlcEnvironment> environment_;
  std::unique_ptr<VlcVideoOutput> video_output_;
  std::unique_ptr<PlayerEventDelegate> event_delegate_;
  VLC::MediaPlayer media_player_;
  std::unique_ptr<VLC::MediaPlayerEventManager> player_event_manager_;

  void SetupEventHandlers() {
    player_event_manager_ = std::make_unique<VLC::MediaPlayerEventManager>(
        media_player_.eventManager());

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

    player_event_manager_->onEncounteredError(
        [this] { HandleVlcState(PlaybackState::kError); });

    player_event_manager_->onMediaChanged(
        [this](VLC::MediaPtr media) { HandleMediaChanged(std::move(media)); });

    player_event_manager_->onLengthChanged(
        [this](int64_t length) { HandleLengthChanged(length); });

    player_event_manager_->onPositionChanged(
        [this](double position) { HandlePositionChanged(position); });

    player_event_manager_->onSeekableChanged(
        [this](bool is_seekable) { HandleSeekableChanged(is_seekable); });

    player_event_manager_->onAudioVolume(
        [this](float value) { HandleVolumeChanged(value); });

    player_event_manager_->onMuted([this]() { HandleMuteChanged(true); });
    player_event_manager_->onUnmuted([this]() { HandleMuteChanged(false); });
  }

  void OnPlay() {
    // Ensure media_state_ is in sync with the actual state.
    HandleMuteChanged(media_player_.mute());
    HandleVolumeChanged(media_player_.volume() / 100.0f);
  }

  void HandleVlcState(PlaybackState state) {
    bool has_change = false;
    bool restart_playback = false;

    PLAYER_LOG("STATE IS " << PlaybackStateToString(state));

    MediaPlaybackPosition position;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);

      if (media_state_.playback_state != state) {
        media_state_.playback_state = state;
        switch (state) {
          case PlaybackState::kStopped:
            media_state_.position = 0;
            // Only restart playback unless there was an error to prevent
            // an infinite loop.
            restart_playback =
                !media_state_.has_error && state_.loop_mode == kLoop;
            break;
          case PlaybackState::kEnded:
            media_state_.position = 0;
            break;
          case PlaybackState::kError:
            media_state_.position = 0;
            media_state_.has_error = true;
            break;
          default:
            break;
        }
        has_change = true;
      }

      position = media_state_.GetPosition();
    }

    if (has_change) {
      if (state == PlaybackState::kPlaying) {
        OnPlay();
      }
      NotifyStateChanged(state);
      NotifyPositionChanged(position);

      if (restart_playback) {
        environment_->task_runner()->Enqueue([weak_self = weak_from_this()]() {
          auto self = weak_self.lock();
          if (self) {
            self->Play();
          }
        });
      }
    }
  }

  void HandleMediaChanged(VLC::MediaPtr vlc_media_ptr) {
    std::unique_ptr<Media> current_media;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      media_state_.duration.reset();
      media_state_.position = 0;
      auto duration = vlc_media_ptr->duration();
      if (duration != -1) {
        media_state_.duration = duration;
      }
      auto vlc_media = vlc_media_ptr->get();

      // Ensure that there was no attempt to open another file in the meantime.
      if (media_state_.media && media_state_.media->vlc_media() == vlc_media) {
        auto media = media_state_.media->media();
        assert(media);
        current_media = std::make_unique<Media>(*media);
      } else {
        std::cerr << "Media has just changed again" << std::endl;
      }
    }

    if (current_media && event_delegate_) {
      event_delegate_->OnMediaChanged(*current_media.get());
    }
  }

  void HandleLengthChanged(int64_t length) {
    MediaPlaybackPosition playback_position;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      media_state_.duration = length;
      playback_position = media_state_.GetPosition();
    }
    NotifyPositionChanged(playback_position);
  }

  void HandlePositionChanged(double position) {
    position = std::clamp(position, 0.0, 1.0);
    MediaPlaybackPosition playback_position;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      media_state_.position = position;
      playback_position = media_state_.GetPosition();
    }
    NotifyPositionChanged(playback_position);
  }

  void HandleSeekableChanged(bool is_seekable) {
    bool has_change = false;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      if (is_seekable != media_state_.is_seekable) {
        media_state_.is_seekable = is_seekable;
        has_change = true;
      }
    }
    if (has_change && event_delegate_) {
      event_delegate_->OnIsSeekableChanged(is_seekable);
    }
  }

  void HandleMuteChanged(bool is_mute) {
    bool has_change = false;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      if (media_state_.is_mute != is_mute) {
        media_state_.is_mute = is_mute;
        has_change = true;
      }
    }
    if (has_change && event_delegate_) {
      event_delegate_->OnMute(is_mute);
    }
  }

  void HandleVolumeChanged(float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    bool has_change = false;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      if (volume != media_state_.volume) {
        media_state_.volume = volume;
        has_change = true;
      }
    }
    if (has_change && event_delegate_) {
      event_delegate_->OnVolumeChanged(volume);
    }
  }

  void NotifyStateChanged(PlaybackState playback_state) {
    if (event_delegate_) {
      event_delegate_->OnPlaybackStateChanged(playback_state);
    }
  }

  void NotifyPositionChanged(const MediaPlaybackPosition& position) {
    if (event_delegate_) {
      event_delegate_->OnPositionChanged(position);
    }
  }
};

}  // namespace foxglove
