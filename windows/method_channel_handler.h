#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "base/task_queue.h"
#include "player_bridge.h"
#include "resource_registry.h"

namespace foxglove {
namespace windows {
class MethodChannelHandler {
 public:
  MethodChannelHandler(
      std::unique_ptr<PlayerResourceRegistry> resource_registry,
      flutter::BinaryMessenger* binary_messenger,
      flutter::TextureRegistrar* texture_registrar,
      winrt::com_ptr<IDXGIAdapter> graphics_adapter);

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void InitPlatform(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void CreateEnvironment(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void DisposeEnvironment(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void CreatePlayer(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void DisposePlayer(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  void Terminate();

  inline bool IsValid() const { return !task_queue_->terminated(); }

 private:
  winrt::com_ptr<IDXGIAdapter> graphics_adapter_;
  flutter::TextureRegistrar* texture_registrar_;
  flutter::BinaryMessenger* binary_messenger_;
  std::unique_ptr<PlayerResourceRegistry> registry_;
  std::shared_ptr<TaskQueue> task_queue_;
  std::shared_ptr<SingleThreadDispatcher> main_thread_dispatcher_;

  int64_t CreateVideoOutput(Player* player);
};
}  // namespace windows
}  // namespace foxglove
