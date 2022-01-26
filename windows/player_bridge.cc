
#include "player_bridge.h"

#include <optional>

#include "base/rref.h"
#include "media/media.h"

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

/*
template <typename T>
std::optional<T> TryGetMapElement(const flutter::EncodableMap* map,
                                  const std::string& key) {
  auto it = map->find(key);
  if (it != map->end()) {
    if (auto value = std::get_if<T>(it->second.get())) {
      return value;
    }
  }
  return std::nullopt;
}
*/

template <typename T>
const T* TryGetMapElement(const flutter::EncodableMap* map,
                          const std::string& key) {
  auto it = map->find(key);
  if (it != map->end()) {
    const T* value = std::get_if<T>(&it->second);
    if (value) {
      return value;
    }
  }
  return nullptr;
}

bool TryConvertPlaylistMode(int32_t value, PlaylistMode& playlist_mode) {
  if (value < PlaylistMode::last_value) {
    playlist_mode = static_cast<PlaylistMode>(value);
    return true;
  }
  return false;
}

std::unique_ptr<Media> TryCreateMedia(const flutter::EncodableMap* map) {
  auto media_type = TryGetMapElement<std::string>(map, "type");
  auto resource = TryGetMapElement<std::string>(map, "resource");

  if (media_type && resource) {
    return Media::create(*media_type, *resource);
  }

  return nullptr;
}

void TryPopulatePlaylist(Playlist* playlist, const flutter::EncodableMap* map,
                         PlaylistMode& playlist_mode) {
  auto medias = TryGetMapElement<flutter::EncodableList>(map, "medias");
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

  if (auto mode = TryGetMapElement<int32_t>(map, "mode")) {
    TryConvertPlaylistMode(*mode, playlist_mode);
  }
}

constexpr auto kMethodOpen = "open";
constexpr auto kMethodPlay = "play";
constexpr auto kMethodPause = "pause";
constexpr auto kMethodStop = "stop";
constexpr auto kMethodNext = "next";
constexpr auto kMethodPrevious = "previous";
constexpr auto kMethodSetPlaylistMode = "setPlaylistMode";

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
      if (auto media_map =
              TryGetMapElement<flutter::EncodableMap>(map, "media")) {
        auto media = TryCreateMedia(media_map);
        if (media) {
          return task_queue_->Enqueue([player = player_,
                                       media_ptr = media.release(),
                                       result = shared_result]() {
            std::unique_ptr<Media> media(media_ptr);
            player->Open(std::move(media));
            result->Success();
          });
        }
      } else if (auto playlist_map =
                     TryGetMapElement<flutter::EncodableMap>(map, "playlist")) {
        auto playlist = player_->CreatePlaylist();
        PlaylistMode mode = PlaylistMode::single;
        TryPopulatePlaylist(playlist.get(), playlist_map, mode);
        return task_queue_->Enqueue([player = player_,
                                     playlist_ptr = playlist.release(), mode,
                                     result = shared_result]() {
          std::unique_ptr<Playlist> playlist(playlist_ptr);
          player->Open(std::move(playlist));
          player->SetPlaylistMode(mode);
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
    return result->Error("bad arguments");
  }

  std::cerr << "GOT METHOD CALL " << method_name << std::endl;
  shared_result->NotImplemented();
}

}  // namespace windows
}  // namespace foxglove