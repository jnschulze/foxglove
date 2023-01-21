#pragma once

#include <condition_variable>
#include <mutex>

#include "events.h"
#include "player.h"
#include "vlc/vlc_environment.h"
#include "vlc/vlc_media_list_player.h"
#include "vlc/vlc_playlist.h"
#include "vlc/vlc_video_output.h"

namespace foxglove {

struct VlcMediaState {
  std::optional<int64_t> duration;
  double position;
  int32_t index;
  PlaybackState playback_state;
  std::optional<int64_t> pending_seek_time;
  // VLC::MediaPtr current_item;
  bool is_seekable;
  bool is_mute;
  float volume;

  VlcMediaState() { Reset(); }

  void Reset() {
    duration.reset();
    position = 0;
    index = 0;
    playback_state = PlaybackState::kNone;
    pending_seek_time.reset();
    // current_item.reset();
    is_seekable = false;
    is_mute = false;
    volume = 0;
  }
};

struct VlcPlayerState {
  bool is_playlist;
  PlaylistMode playlist_mode;

  VlcPlayerState() { Reset(); }

  void Reset() {
    is_playlist = false;
    playlist_mode = PlaylistMode::single;
  }
};

class VlcPlayer : public Player {
 public:
  VlcPlayer(std::shared_ptr<VlcEnvironment> environment);
  ~VlcPlayer() override;

  inline bool IsValid() { return !shutting_down_; }

  void Shutdown();

  void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) override {
    event_delegate_ = std::move(event_delegate);
  }

  // |VideoOutputFactory|
  std::unique_ptr<VideoOutput> CreatePixelBufferOutput(
      std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
      PixelFormat pixel_format) const override;

#ifdef _WIN32
  // |VideoOutputFactory|
  std::unique_ptr<VideoOutput> CreateD3D11Output(
      std::unique_ptr<D3D11OutputDelegate> output_delegate,
      winrt::com_ptr<IDXGIAdapter> adapter = nullptr) const override;
#endif

  void SetVideoOutput(std::unique_ptr<VideoOutput> output) override;
  VideoOutput* GetVideoOutput() const override { return video_output_.get(); }

  std::unique_ptr<Playlist> CreatePlaylist() override;

  void Open(std::unique_ptr<Media> media) override;
  void Open(std::unique_ptr<Playlist> playlist) override;

  void Play() override;
  void Pause() override;
  void Stop() override;
  void StopSync(
      std::optional<std::chrono::milliseconds> timeout = std::nullopt);
  void SeekPosition(float position) override;
  void SeekTime(int64_t time) override;
  void SetRate(float rate);

  void Next() override;
  void Previous() override;

  void SetPlaylistMode(PlaylistMode playlist_mode) override;
  void SetVolume(double volume) override;
  void SetMute(bool is_muted) override;

  int64_t duration() override;

  libvlc_media_player_t* vlc_player() const { return media_player_.get(); }

 private:
  typedef std::function<void()> VoidCallback;
  VlcMediaState media_state_;
  VlcPlayerState state_;
  std::mutex op_mutex_;
  std::mutex state_mutex_;
  bool shutting_down_ = false;

  std::mutex stop_mutex_;
  std::condition_variable stop_cond_;
  bool is_stopped_ = false;

  std::shared_ptr<VlcEnvironment> environment_;
  std::unique_ptr<VlcVideoOutput> video_output_;
  std::unique_ptr<PlayerEventDelegate> event_delegate_;
  VLC::MediaPlayer media_player_;
  std::shared_ptr<VlcMediaListPlayer> media_list_player_;
  std::unique_ptr<VLC::MediaPlayerEventManager> player_event_manager_;

  void OpenInternal(std::unique_ptr<VlcPlaylist> playlist, bool is_playlist);
  void PlayInternal();
  void PauseInternal();
  bool StopInternal();
  void StopSyncInternal(
      std::optional<std::chrono::milliseconds> timeout = std::nullopt);
  void SetPlaylistModeInternal(PlaylistMode playlist_mode);
  void OnPlaylistUpdated();

  void SafeInvoke(VoidCallback callback);

  void SetupEventHandlers();
  void OnPlay();
  void HandleVlcState(PlaybackState state);
  void HandleMediaChanged(VLC::MediaPtr vlc_media_ptr);
  void HandleLengthChanged(int64_t length);
  void HandlePositionChanged(float relative_position);
  void HandleSeekableChanged(bool is_seekable);
  void HandleMuteChanged(bool is_mute);
  void HandleVolumeChanged(float volume);
  void NotifyStateChanged();
  void NotifyMediaChanged();
  void NotifyPositionChanged();
  void Subscribe(VLC::EventManager::RegisteredEvent ev);
};

}  // namespace foxglove