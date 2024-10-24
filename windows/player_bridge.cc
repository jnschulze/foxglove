
#include "player_bridge.h"

#include "base/make_copyable.h"
#include "media/media.h"
#include "method_channel_utils.h"
#include "player_events.h"

namespace foxglove {
namespace windows {

namespace {

bool TryConvertLoopMode(int32_t value, LoopMode& loop_mode) {
  if (value < LoopMode::kLastValue) {
    loop_mode = static_cast<LoopMode>(value);
    return true;
  }
  return false;
}

std::unique_ptr<Media> TryCreateMedia(const flutter::EncodableMap* map) {
  auto media_type = channels::TryGetMapElement<std::string>(map, "type");
  auto resource = channels::TryGetMapElement<std::string>(map, "resource");

  if (media_type && resource) {
    return Media::create(*media_type, *resource);
  }

  return nullptr;
}

constexpr auto kMethodOpen = "open";
constexpr auto kMethodClose = "close";
constexpr auto kMethodPlay = "play";
constexpr auto kMethodPause = "pause";
constexpr auto kMethodStop = "stop";
constexpr auto kMethodSeekPosition = "seekPosition";
constexpr auto kMethodSeekTime = "seekTime";
constexpr auto kMethodSetRate = "setRate";
constexpr auto kMethodSetLoopMode = "setLoopMode";
constexpr auto kMethodSetVolume = "setVolume";
constexpr auto kMethodMute = "mute";
constexpr auto kMethodUnmute = "unmute";

constexpr auto kErrorCodeBadArgs = "invalid_arguments";
constexpr auto kErrorCodePluginTerminated = "plugin_terminated";
constexpr auto kErrorVlc = "vlc_error";

}  // namespace

PlayerBridge::PlayerBridge(
    flutter::BinaryMessenger* messenger, std::shared_ptr<TaskQueue> task_queue,
    PlayerRegistry::PlayerType* player,
    std::shared_ptr<MainThreadDispatcher> main_thread_dispatcher)
    : player_(player), task_queue_(std::move(task_queue)) {
  channels_ = std::make_shared<PlayerChannels>(
      messenger, player->id(), std::move(main_thread_dispatcher));
}

void PlayerBridge::RegisterChannelHandlers(Closure callback) const {
  channels_->Register(
      [this](const auto& call, auto result) {
        HandleMethodCall(call, std::move(result));
      },
      std::move(callback));
}

bool PlayerBridge::UnregisterChannelHandlers(Closure callback) const {
  return channels_->Unregister(std::move(callback));
}

void PlayerBridge::Enqueue(
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
        method_result,
    std::function<void(MethodResult result)> handler) const {
  MethodResult shared_result = std::move(method_result);
  if (!task_queue_->Enqueue([shared_result, handler = std::move(handler)]() {
        handler(shared_result);
      })) {
    shared_result->Error(kErrorCodePluginTerminated);
  }
}

void PlayerBridge::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
    const {
  if (!IsValid()) {
    result->Error(kErrorCodePluginTerminated);
    return;
  }

  const auto& method_name = method_call.method_name();

  if (method_name.compare(kMethodOpen) == 0) {
    if (auto map =
            std::get_if<flutter::EncodableMap>(method_call.arguments())) {
      bool autostart = false;
      if (auto flag = channels::TryGetMapElement<bool>(map, "autostart")) {
        autostart = *flag;
      }

      if (auto media_map =
              channels::TryGetMapElement<flutter::EncodableMap>(map, "media")) {
        auto media = TryCreateMedia(media_map);
        if (media) {
          return Enqueue(
              std::move(result),
              MakeCopyable([player = player_, media = std::move(media),
                            autostart](MethodResult result) mutable {
                if (!player->Open(std::move(media))) {
                  return result->Error(kErrorVlc, "Failed to open media");
                }
                if (autostart) {
                  if (!player->Play()) {
                    return result->Error(kErrorVlc, "Failed to play media");
                  }
                }
                result->Success();
              }));
        }
      }
    }

    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodClose) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      if (!player->Open(nullptr)) {
        return result->Error(kErrorVlc, "Failed to close media");
      }
      result->Success();
    });
  }

  if (method_name.compare(kMethodPlay) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      if (!player->Play()) {
        return result->Error(kErrorVlc, "Failed to start player");
      }
      result->Success();
    });
  }

  if (method_name.compare(kMethodPause) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      player->Pause();
      result->Success();
    });
  }

  if (method_name.compare(kMethodStop) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      player->Stop();
      result->Success();
    });
  }

  if (method_name.compare(kMethodSeekPosition) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return Enqueue(std::move(result),
                     [player = player_, value = *value](MethodResult result) {
                       player->SeekPosition(static_cast<float>(value));
                       result->Success();
                     });
    }
    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodSeekTime) == 0) {
    if (auto value = channels::TryGetIntValue(method_call.arguments())) {
      return Enqueue(
          std::move(result),
          [player = player_, value = value.value()](MethodResult result) {
            player->SeekTime(value);
            result->Success();
          });
    }
    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodSetRate) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return Enqueue(std::move(result),
                     [player = player_, value = *value](MethodResult result) {
                       player->SetRate(static_cast<float>(value));
                       result->Success();
                     });
    }
    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodSetLoopMode) == 0) {
    if (auto value = std::get_if<int32_t>(method_call.arguments())) {
      LoopMode mode = LoopMode::kOff;
      if (TryConvertLoopMode(*value, mode)) {
        return Enqueue(std::move(result),
                       [player = player_, mode](MethodResult result) {
                         player->SetLoopMode(mode);
                         result->Success();
                       });
      }
    }
    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodSetVolume) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return Enqueue(std::move(result),
                     [player = player_, value = *value](MethodResult result) {
                       player->SetVolume(value);
                       result->Success();
                     });
    }
    return result->Error(kErrorCodeBadArgs);
  }

  if (method_name.compare(kMethodMute) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      player->SetMute(true);
      result->Success();
    });
  }

  if (method_name.compare(kMethodUnmute) == 0) {
    return Enqueue(std::move(result), [player = player_](MethodResult result) {
      player->SetMute(false);
      result->Success();
    });
  }

  std::cerr << "Got unhandled method call: " << method_name << std::endl;
  assert(result);
  result->NotImplemented();
}

void PlayerBridge::EmitEvent(
    std::unique_ptr<flutter::EncodableValue> event) const {
  channels_->EmitEvent(std::move(event));
}

void PlayerBridge::OnMediaChanged(std::unique_ptr<Media> media) {
  if (!media) {
    EmitEvent(channels::MakeValue(
        flutter::EncodableMap{{kEventType, Events::kMediaChanged}}));
    return;
  }

  EmitEvent(channels::MakeValue(
      flutter::EncodableMap{{kEventType, Events::kMediaChanged},
                            {"type", media->media_type()},
                            {"resource", media->resource()}}));
}

void PlayerBridge::OnPlaybackStateChanged(PlaybackState state) {
  EmitEvent(channels::MakeValue(
      flutter::EncodableMap{{kEventType, kPlaybackStateChanged},
                            {"state", static_cast<int32_t>(state)}}));
}

void PlayerBridge::OnIsSeekableChanged(bool is_seekable) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kIsSeekableChanged},
      {kEventValue, is_seekable},
  }));
}

void PlayerBridge::OnPositionChanged(const MediaPlaybackPosition& position) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kPositionChanged},
      {"duration", position.duration},
      {"position", position.position},
  }));
}

void PlayerBridge::OnRateChanged(double rate) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kRateChanged},
      {kEventValue, rate},
  }));
}

void PlayerBridge::OnVolumeChanged(double volume) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kVolumeChanged},
      {kEventValue, volume},
  }));
}

void PlayerBridge::OnMute(bool is_muted) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kMuteChanged},
      {kEventValue, is_muted},
  }));
}

void PlayerBridge::OnVideoDimensionsChanged(int32_t width, int32_t height) {
  EmitEvent(channels::MakeValue(flutter::EncodableMap{
      {kEventType, kVideoDimensionsChanged},
      {"width", width},
      {"height", height},
  }));
}

}  // namespace windows
}  // namespace foxglove
