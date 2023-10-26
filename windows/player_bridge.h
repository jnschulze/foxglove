#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>

#include <memory>
#include <mutex>

#include "base/task_queue.h"
#include "main_thread_dispatcher.h"
#include "player.h"
#include "plugin_state.h"

namespace foxglove {
namespace windows {

class PlayerBridge : public PlayerEventDelegate {
 public:
  PlayerBridge(flutter::BinaryMessenger* messenger,
               std::shared_ptr<TaskQueue> task_queue, Player* player,
               std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher);
  ~PlayerBridge() override;

  void OnMediaChanged(const Media* media, std::unique_ptr<MediaInfo> media_info,
                      size_t index) override;
  void OnPlaybackStateChanged(PlaybackState playback_state,
                              bool is_seekable) override;
  void OnPositionChanged(double position, int64_t duration) override;
  void OnRateChanged(double rate) override;
  void OnVolumeChanged(double volume) override;
  void OnMute(bool is_muted) override;
  void OnVideoDimensionsChanged(int32_t width, int32_t height) override;

  inline bool IsValid() const { return !task_queue_->terminated(); }
  inline bool IsMessengerValid() const { return PluginState::IsValid(); }

 private:
  typedef std::function<void()> VoidCallback;
  Player* player_;
  std::shared_ptr<TaskQueue> task_queue_;
  std::mutex event_sink_mutex_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher_;

  // Asynchronously registers the channel handlers on the main thread.
  void RegisterChannelHandlers(VoidCallback callback);
  // Asynchronously unregisters the channel handlers on the main thread.
  void UnregisterChannelHandlers(VoidCallback callback);

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  void EmitEvent(const flutter::EncodableValue& event);

  typedef std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      MethodResult;
  void Enqueue(std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
                   method_result,
               std::function<void(MethodResult result)> handler);
};

}  // namespace windows
}  // namespace foxglove
