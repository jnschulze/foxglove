
#include "player_channels.h"

#include <flutter/event_stream_handler_functions.h>

#include "base/logging.h"
#include "base/make_copyable.h"
#include "player_events.h"

namespace foxglove {
namespace windows {

namespace {

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
  if (size <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

}  // namespace

PlayerChannels::PlayerChannels(
    flutter::BinaryMessenger* messenger, int64_t player_id,
    std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher)
    : main_thread_dispatcher_(std::move(main_thread_dispatcher)) {
  auto method_channel_name = string_format("foxglove/%I64i", player_id);
  method_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          messenger, method_channel_name,
          &flutter::StandardMethodCodec::GetInstance());

  const auto event_channel_name =
      string_format("foxglove/%I64i/events", player_id);
  event_channel_ =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, event_channel_name,
          &flutter::StandardMethodCodec::GetInstance());
}

void PlayerChannels::Register(
    flutter::MethodCallHandler<flutter::EncodableValue> method_call_handler,
    Closure callback) {
  assert(thread_checker_.IsCreationThreadCurrent());

  {
    const std::lock_guard lock(method_call_handler_mutex_);
    method_call_handler_ = method_call_handler;
  }

  main_thread_dispatcher_->Dispatch([self = shared_from_this(),
                                     callback = std::move(callback)]() {
    assert(IsMessengerValid());
    self->method_channel_->SetMethodCallHandler(
        [=](const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
                result) {
          self->HandleMethodCall(method_call, std::move(result));
        });

    auto stream_handler = std::make_unique<
        flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
        [self](const flutter::EncodableValue* arguments,
               std::unique_ptr<flutter::EventSink<flutter::EncodableValue>>&&
                   events) {
          self->SetEventSink(std::move(events));
          return nullptr;
        },
        [self](const flutter::EncodableValue* arguments) {
          self->SetEventSink(nullptr);
          return nullptr;
        });
    self->event_channel_->SetStreamHandler(std::move(stream_handler));

    if (callback) {
      callback();
    }
  });
}

bool PlayerChannels::Unregister(Closure callback) {
  assert(thread_checker_.IsCreationThreadCurrent());
  {
    const std::lock_guard lock(method_call_handler_mutex_);
    method_call_handler_ = nullptr;
  }

  SetEventSink(nullptr);

  if (!IsMessengerValid()) {
    return false;
  }

  main_thread_dispatcher_->Dispatch(
      [self = shared_from_this(), callback = std::move(callback)]() {
        // Channel handlers must not be unset during plugin destruction
        // See https://github.com/flutter/flutter/issues/118611
        if (IsMessengerValid()) {
          self->method_channel_->SetMethodCallHandler(nullptr);
          self->event_channel_->SetStreamHandler(nullptr);
        }

        if (callback) {
          callback();
        }
      });

  return true;
}

void PlayerChannels::EmitEvent(
    std::unique_ptr<flutter::EncodableValue> event) const {
  main_thread_dispatcher_->Dispatch(MakeCopyable(
      [this, weak_self = weak_from_this(), event = std::move(event)] {
        if (!IsMessengerValid()) {
          return;
        }

        if (auto self = weak_self.lock()) {
          const std::shared_lock lock(event_sink_mutex_);
          if (event_sink_) {
            if (event) {
              event_sink_->Success(*event);
            } else {
              event_sink_->Success();
            }
          }
        }
      }));
}

void PlayerChannels::SetEventSink(
    std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink) {
  const std::lock_guard lock(event_sink_mutex_);
  event_sink_ = std::move(event_sink);
  if (event_sink_) {
    event_sink_->Success(flutter::EncodableValue(
        flutter::EncodableMap{{kEventType, Events::kInitialized}}));
  }
}

void PlayerChannels::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const std::shared_lock lock(method_call_handler_mutex_);

  LOG(TRACE) << "Got method call: " << call.method_name() << std::endl;
  if (method_call_handler_) {
    method_call_handler_(call, std::move(result));
  } else {
    LOG(WARNING) << "No handler anymore" << std::endl;
  }
}

}  // namespace windows
}  // namespace foxglove
