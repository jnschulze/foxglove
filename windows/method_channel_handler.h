#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <unordered_map>

#include "base/task_queue.h"
#include "globals.h"
#include "player_bridge.h"

namespace foxglove {
namespace windows {
class MethodChannelHandler {
 public:
  MethodChannelHandler(ObjectRegistry *object_registry, flutter::BinaryMessenger* binary_messenger);

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  void CreateEnvironment(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void DisposeEnvironment(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void CreatePlayer(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void DisposePlayer(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

 private:
  ObjectRegistry *registry_;
  flutter::BinaryMessenger* binary_messenger_;
  //std::unordered_map<int64_t, std::unique_ptr<PlayerBridge>> players_;
  std::unique_ptr<TaskQueue> task_queue_;
};
}  // namespace windows
}  // namespace foxglove
