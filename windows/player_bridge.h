#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>

#include <memory>
#include <mutex>

#include "base/task_queue.h"
#include "flutter_task_runner.h"
#include "player.h"

namespace foxglove {
namespace windows {

class PlayerBridge : public PlayerEventDelegate {
 public:
  PlayerBridge(flutter::BinaryMessenger* messenger,
               FlutterTaskRunner* platform_task_runner,
               std::shared_ptr<TaskQueue> task_queue, Player* player);
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

 private:
  Player* player_;
  FlutterTaskRunner* platform_task_runner_;
  std::shared_ptr<TaskQueue> task_queue_;
  std::mutex event_sink_mutex_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  void EmitEvent(const flutter::EncodableValue& event);
};

}  // namespace windows
}  // namespace foxglove