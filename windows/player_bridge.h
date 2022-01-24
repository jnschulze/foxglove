#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>

#include <memory>

#include "base/task_queue.h"
#include "player.h"

namespace foxglove {
namespace windows {

class PlayerBridge : public PlayerEventDelegate {
 public:
  PlayerBridge(flutter::BinaryMessenger* messenger, Player* player);
  ~PlayerBridge() override;

 private:
  Player* player_;
  std::unique_ptr<TaskQueue> task_queue_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace windows
}  // namespace foxglove