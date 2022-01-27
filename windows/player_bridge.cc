
#include "player_bridge.h"

#include <flutter/event_stream_handler_functions.h>

#include <optional>

#include "base/rref.h"
#include "media/media.h"
#include "method_channel_utils.h"

// https://stackoverflow.com/questions/8640393/move-capture-in-lambda

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

bool TryConvertPlaylistMode(int32_t value, PlaylistMode& playlist_mode) {
  if (value < PlaylistMode::last_value) {
    playlist_mode = static_cast<PlaylistMode>(value);
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

void TryPopulatePlaylist(Playlist* playlist, const flutter::EncodableMap* map,
                         PlaylistMode& playlist_mode) {
  auto medias =
      channels::TryGetMapElement<flutter::EncodableList>(map, "medias");
  if (medias) {
    for (const auto& item : *medias) {
      if (auto media_map = std::get_if<flutter::EncodableMap>(&item)) {
        auto media = TryCreateMedia(media_map);
        if (media) {
          playlist->Add(std::move(media));
        }
      }
    }
  }

  if (auto mode = channels::TryGetMapElement<int32_t>(map, "mode")) {
    TryConvertPlaylistMode(*mode, playlist_mode);
  }
}

constexpr auto kMethodOpen = "open";
constexpr auto kMethodPlay = "play";
constexpr auto kMethodPause = "pause";
constexpr auto kMethodStop = "stop";
constexpr auto kMethodNext = "next";
constexpr auto kMethodPrevious = "previous";
constexpr auto kMethodSeekPosition = "seekPosition";
constexpr auto kMethodSeekTime = "seekTime";
constexpr auto kMethodSetRate = "setRate";
constexpr auto kMethodSetPlaylistMode = "setPlaylistMode";
constexpr auto kMethodSetVolume = "setVolume";
constexpr auto kMethodMute = "mute";
constexpr auto kMethodUnmute = "unmute";

constexpr auto kEventType = "type";
constexpr auto kEventValue = "value";

enum Events : int32_t {
  kNone,
  kPositionChanged,
  kPlaybackStateChanged,
  kMediaChanged,
  kRateChanged,
  kVolumeChanged,
  kMuteChanged,
  kVideoDimensionsChanged
};

}  // namespace

PlayerBridge::PlayerBridge(flutter::BinaryMessenger* messenger, Player* player)
    : player_(player), task_queue_(std::make_unique<TaskQueue>()) {
  auto method_channel_name = string_format("foxglove/%I64i", player->id());
  method_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          messenger, method_channel_name,
          &flutter::StandardMethodCodec::GetInstance());
  method_channel_->SetMethodCallHandler([this](const auto& call, auto result) {
    HandleMethodCall(call, std::move(result));
  });

  const auto event_channel_name =
      string_format("foxglove/%I64i/events", player->id());
  event_channel_ =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, event_channel_name,
          &flutter::StandardMethodCodec::GetInstance());

  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [this](const flutter::EncodableValue* arguments,
             std::unique_ptr<flutter::EventSink<flutter::EncodableValue>>&&
                 events) {
        event_sink_ = std::move(events);
        return nullptr;
      },
      [](const flutter::EncodableValue* arguments) { return nullptr; });

  event_channel_->SetStreamHandler(std::move(handler));
}

PlayerBridge::~PlayerBridge() {
  method_channel_->SetMethodCallHandler(nullptr);
}

void PlayerBridge::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto& method_name = method_call.method_name();

  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

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
          return task_queue_->Enqueue([player = player_,
                                       media_ptr = media.release(), autostart,
                                       result = shared_result]() {
            std::unique_ptr<Media> media(media_ptr);
            player->Open(std::move(media));
            if (autostart) {
              player->Play();
            }
            result->Success();
          });
        }
      } else if (auto playlist_map =
                     channels::TryGetMapElement<flutter::EncodableMap>(
                         map, "playlist")) {
        auto playlist = player_->CreatePlaylist();
        PlaylistMode mode = PlaylistMode::single;
        TryPopulatePlaylist(playlist.get(), playlist_map, mode);
        return task_queue_->Enqueue([player = player_,
                                     playlist_ptr = playlist.release(), mode,
                                     autostart, result = shared_result]() {
          std::unique_ptr<Playlist> playlist(playlist_ptr);
          player->Open(std::move(playlist));
          player->SetPlaylistMode(mode);
          if (autostart) {
            player->Play();
          }
          result->Success();
        });
      }
    }

    return shared_result->Error("invalid args");
  }

  if (method_name.compare(kMethodPlay) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->Play();
      result->Success();
    });
  }

  if (method_name.compare(kMethodPause) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->Pause();
      result->Success();
    });
  }

  if (method_name.compare(kMethodStop) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->Stop();
      result->Success();
    });
  }

  if (method_name.compare(kMethodNext) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->Next();
      result->Success();
    });
  }

  if (method_name.compare(kMethodPrevious) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->Previous();
      result->Success();
    });
  }

  if (method_name.compare(kMethodSeekPosition) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return task_queue_->Enqueue(
          [player = player_, value = *value, result = shared_result]() {
            player->SeekPosition(static_cast<float>(value));
            result->Success();
          });
    }
    return shared_result->Error("bad arguments");
  }

  if (method_name.compare(kMethodSeekTime) == 0) {
    if (auto value = channels::TryGetIntValue(method_call.arguments())) {
      return task_queue_->Enqueue(
          [player = player_, value = value.value(), result = shared_result]() {
            player->SeekTime(value);
            result->Success();
          });
    }
    return shared_result->Error("bad arguments");
  }

  if (method_name.compare(kMethodSetRate) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return task_queue_->Enqueue(
          [player = player_, value = *value, result = shared_result]() {
            player->SetRate(static_cast<float>(value));
            result->Success();
          });
    }
    return shared_result->Error("bad arguments");
  }

  if (method_name.compare(kMethodSetPlaylistMode) == 0) {
    if (auto value = std::get_if<int32_t>(method_call.arguments())) {
      PlaylistMode mode = PlaylistMode::single;
      if (TryConvertPlaylistMode(*value, mode)) {
        return task_queue_->Enqueue(
            [player = player_, mode, result = shared_result]() {
              player->SetPlaylistMode(mode);
              result->Success();
            });
      }
    }
    return shared_result->Error("bad arguments");
  }

  if (method_name.compare(kMethodSetVolume) == 0) {
    if (auto value = std::get_if<double>(method_call.arguments())) {
      return task_queue_->Enqueue(
          [player = player_, value = *value, result = shared_result]() {
            player->SetVolume(value);
            result->Success();
          });
    }
    return shared_result->Error("bad arguments");
  }

  if (method_name.compare(kMethodMute) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->SetMute(true);
      result->Success();
    });
  }

  if (method_name.compare(kMethodUnmute) == 0) {
    return task_queue_->Enqueue([player = player_, result = shared_result]() {
      player->SetMute(false);
      result->Success();
    });
  }

  std::cerr << "GOT METHOD CALL " << method_name << std::endl;
  shared_result->NotImplemented();
}

void PlayerBridge::EmitEvent(const flutter::EncodableValue& event) {
  if (event_sink_) {
    event_sink_->Success(event);
  }
}

void PlayerBridge::OnMediaChanged(const Media* media,
                                  std::unique_ptr<MediaInfo> media_info,
                                  size_t index) {
  //
  //

  const auto event = flutter::EncodableValue(
      flutter::EncodableMap{{kEventType, Events::kMediaChanged},
                            {"media",
                             flutter::EncodableMap{
                                 {"type", media->media_type()},
                                 {"resource", media->resource()},
                             }},
                            {"duration", media_info->duration()},
                            {"index", static_cast<int32_t>(index)}});
  EmitEvent(event);
}

void PlayerBridge::OnPlaybackStateChanged(PlaybackState state,
                                          bool is_seekable) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kPlaybackStateChanged},
      {"state", static_cast<int32_t>(state)},
      {"is_seekable", is_seekable},
  });
  EmitEvent(event);
}

void PlayerBridge::OnPositionChanged(double position, int64_t duration) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kPositionChanged},
      {"duration", duration},
      {"position", position},
  });
  EmitEvent(event);
}

void PlayerBridge::OnRateChanged(double rate) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kRateChanged},
      {kEventValue, rate},
  });
  EmitEvent(event);
}

void PlayerBridge::OnVolumeChanged(double volume) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kVolumeChanged},
      {kEventValue, volume},
  });
  EmitEvent(event);
}

void PlayerBridge::OnMute(bool is_muted) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kMuteChanged},
      {kEventValue, is_muted},
  });
  EmitEvent(event);
}

void PlayerBridge::OnVideoDimensionsChanged(int32_t width, int32_t height) {
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {kEventType, kVideoDimensionsChanged},
      {"width", width},
      {"height", height},
  });
  EmitEvent(event);
}

}  // namespace windows
}  // namespace foxglove