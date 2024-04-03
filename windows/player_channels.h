#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <mutex>

#include "base/closure.h"
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
  void EmitEvent(const flutter::EncodableValue& event) const;

 private:
  mutable std::mutex event_sink_mutex_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
  std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher_;

  inline static bool IsMessengerValid() { return PluginState::IsValid(); }
  void SetEventSink(
      std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink);
};

}  // namespace windows
}  // namespace foxglove
