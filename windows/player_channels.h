#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <shared_mutex>

#include "base/closure.h"
#include "base/thread_checker.h"
#include "main_thread_dispatcher.h"
#include "plugin_state.h"

namespace foxglove {
namespace windows {

class PlayerChannels : public std::enable_shared_from_this<PlayerChannels> {
 public:
  PlayerChannels(flutter::BinaryMessenger* messenger, int64_t player_id,
                 std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher);
  void Register(flutter::MethodCallHandler<flutter::EncodableValue> handler,
                Closure callback);
  bool Unregister(Closure callback);
  void EmitEvent(std::unique_ptr<flutter::EncodableValue> event) const;

 private:
  ThreadChecker thread_checker_;
  mutable std::shared_mutex event_sink_mutex_;
  std::shared_mutex method_call_handler_mutex_;
  flutter::MethodCallHandler<flutter::EncodableValue> method_call_handler_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
  std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher_;

  inline static bool IsMessengerValid() { return PluginState::IsValid(); }
  void SetEventSink(
      std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink);
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace windows
}  // namespace foxglove
