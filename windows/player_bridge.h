#pragma once

#include "base/task_queue.h"
#include "main_thread_dispatcher.h"
#include "player.h"
#include "player_channels.h"

namespace foxglove {
namespace windows {

class PlayerBridge : public PlayerEventDelegate {
 public:
  PlayerBridge(flutter::BinaryMessenger* messenger,
               std::shared_ptr<TaskQueue> task_queue, Player* player,
               std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher);

  // Asynchronously registers the channel handlers on the main thread.
  void RegisterChannelHandlers(Closure callback) const;
  // Asynchronously unregisters the channel handlers on the main thread.
  bool UnregisterChannelHandlers(Closure callback) const;

  void OnMediaChanged(std::unique_ptr<Media> media) override;
  void OnPlaybackStateChanged(PlaybackState playback_state) override;
  void OnIsSeekableChanged(bool is_seekable) override;
  void OnPositionChanged(const MediaPlaybackPosition& position) override;
  void OnRateChanged(double rate) override;
  void OnVolumeChanged(double volume) override;
  void OnMute(bool is_muted) override;
  void OnVideoDimensionsChanged(int32_t width, int32_t height) override;

  inline bool IsValid() const { return !task_queue_->terminated(); }

 private:
  Player* player_;
  std::shared_ptr<TaskQueue> task_queue_;
  std::shared_ptr<PlayerChannels> channels_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
      const;
  void EmitEvent(std::unique_ptr<flutter::EncodableValue> event) const;
  typedef std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      MethodResult;

  void Enqueue(std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
                   method_result,
               std::function<void(MethodResult result)> handler) const;
};

}  // namespace windows
}  // namespace foxglove
