#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "base/task_queue.h"
#include "main_thread_dispatcher.h"
#include "player_registry.h"
#include "third_party/expected.h"
#include "video/texture_registry.h"

namespace foxglove {
namespace windows {
class MethodChannelHandler {
 public:
  typedef PlayerRegistry::PlayerType PlayerType;

  MethodChannelHandler(std::unique_ptr<PlayerRegistry> registry,
                       flutter::BinaryMessenger* binary_messenger,
                       flutter::TextureRegistrar* texture_registrar,
                       winrt::com_ptr<IDXGIAdapter> graphics_adapter);

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void InitPlatform(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  void ConfigureLogging(
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
  std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher_;
  winrt::com_ptr<IDXGIAdapter> graphics_adapter_;
  std::unique_ptr<TextureRegistry> texture_registry_;
  flutter::BinaryMessenger* binary_messenger_;
  std::unique_ptr<PlayerRegistry> registry_;
  std::shared_ptr<TaskQueue> task_queue_;

  tl::expected<int64_t, ErrorDetails> CreateVideoOutput(PlayerType* player);

  void DestroyPlayers();

  // Asynchronously unregisters channel handlers.
  // Returns true if the callback, if provided, is guaranteed to be invoked.
  bool UnregisterChannelHandlers(PlayerType* player,
                                 Closure callback = nullptr);
};
}  // namespace windows
}  // namespace foxglove
