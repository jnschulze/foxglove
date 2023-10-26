#include "method_channel_handler.h"

#include <future>

#include "method_channel_utils.h"
#include "player_environment.h"
#include "vlc/vlc_environment.h"

#ifdef HAVE_FLUTTER_D3D_TEXTURE
#include "video/video_outlet_d3d.h"
#else
#include "video/video_outlet.h"
#endif

namespace foxglove {
namespace windows {

namespace {
constexpr auto kMethodInitPlatform = "init";
constexpr auto kMethodCreateEnvironment = "createEnvironment";
constexpr auto kMethodDisposeEnvironment = "disposeEnvironment";
constexpr auto kMethodCreatePlayer = "createPlayer";
constexpr auto kMethodDisposePlayer = "disposePlayer";

constexpr auto kErrorCodeInvalidArguments = "invalid_args";
constexpr auto kErrorCodeInvalidId = "invalid_id";
constexpr auto kErrorCodePluginTerminated = "plugin_terminated";

}  // namespace

MethodChannelHandler::MethodChannelHandler(
    std::unique_ptr<PlayerResourceRegistry> resource_registry,
    flutter::BinaryMessenger* binary_messenger,
    flutter::TextureRegistrar* texture_registrar,
    winrt::com_ptr<IDXGIAdapter> graphics_adapter)
    : registry_(std::move(resource_registry)),
      binary_messenger_(binary_messenger),
      texture_registrar_(texture_registrar),
      graphics_adapter_(std::move(graphics_adapter)),
      task_queue_(std::make_shared<TaskQueue>(
          1, "io.jns.foxglove.methodchannelhandler")),
      main_thread_dispatcher_(std::make_shared<MainThreadDispatcher>()) {}

void MethodChannelHandler::Terminate() {
  if (!IsValid()) {
    return;
  }

  std::promise<void> promise;
  task_queue_->Enqueue([&]() {
    registry_->players()->Clear();
    registry_->environments()->Clear();

    task_queue_->Terminate();
    promise.set_value();
  });

  promise.get_future().wait();

  // make sure we make a last call to the main thread dispatcher to finish
  // pending work
  main_thread_dispatcher_->Terminate();
}

void MethodChannelHandler::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (!IsValid()) {
    result->Error(kErrorCodePluginTerminated);
    return;
  }

  const auto& method_name = method_call.method_name();

  if (method_name.compare(kMethodInitPlatform) == 0) {
    return InitPlatform(method_call, std::move(result));
  }

  if (method_name.compare(kMethodCreateEnvironment) == 0) {
    return CreateEnvironment(method_call, std::move(result));
  }

  if (method_name.compare(kMethodDisposeEnvironment) == 0) {
    return DisposeEnvironment(method_call, std::move(result));
  }

  if (method_name.compare(kMethodCreatePlayer) == 0) {
    return CreatePlayer(method_call, std::move(result));
  }

  if (method_name.compare(kMethodDisposePlayer) == 0) {
    return DisposePlayer(method_call, std::move(result));
  }

  result->NotImplemented();
}

void MethodChannelHandler::InitPlatform(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

  if (!task_queue_->Enqueue([this, shared_result]() {
        // Clear old instances after hot restart.
        registry_->players()->Clear();
        registry_->environments()->Clear();
        shared_result->Success();
      })) {
    shared_result->Error(kErrorCodePluginTerminated);
  }
}

void MethodChannelHandler::CreateEnvironment(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::vector<std::string> env_args;
  if (auto map = std::get_if<flutter::EncodableMap>(method_call.arguments())) {
    if (auto list =
            channels::TryGetMapElement<flutter::EncodableList>(map, "args")) {
      channels::GetStringList(list, env_args);
    }
  }

  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);
  if (!task_queue_->Enqueue(
          [this, args = std::move(env_args), shared_result]() {
            auto env = std::make_shared<foxglove::VlcEnvironment>(args);
            auto id = env->id();
            registry_->environments()->RegisterEnvironment(id, std::move(env));
            shared_result->Success(id);
          })) {
    shared_result->Error(kErrorCodePluginTerminated);
  }
}

void MethodChannelHandler::DisposeEnvironment(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

  if (auto id = std::get_if<int64_t>(method_call.arguments())) {
    if (!task_queue_->Enqueue(
            [id = *id, shared_result, registry = registry_.get()]() {
              if (registry->environments()->RemoveEnvironment(id)) {
                shared_result->Success();
              } else {
                shared_result->Error(kErrorCodeInvalidId);
              }
            })) {
      shared_result->Error(kErrorCodePluginTerminated);
    }
  } else {
    shared_result->Error(kErrorCodeInvalidArguments);
  }
}

void MethodChannelHandler::CreatePlayer(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::optional<int64_t> environment_id;
  std::vector<std::string> environment_args;
  if (auto map = std::get_if<flutter::EncodableMap>(method_call.arguments())) {
    if (auto id = channels::TryGetMapElement<int64_t>(map, "environmentId")) {
      environment_id = *id;
    } else if (auto args = channels::TryGetMapElement<flutter::EncodableList>(
                   map, "environmentArgs")) {
      channels::GetStringList(args, environment_args);
    }
  }

  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

  if (!task_queue_->Enqueue([environment_id,
                             env_args = std::move(environment_args),
                             shared_result, this]() {
        std::shared_ptr<foxglove::PlayerEnvironment> env;
        if (environment_id.has_value()) {
          env =
              registry_->environments()->GetEnvironment(environment_id.value());
          if (!env) {
            return shared_result->Error(kErrorCodeInvalidId);
          }
        } else {
          env = std::make_shared<foxglove::VlcEnvironment>(env_args);
          if (!env) {
            return shared_result->Error("env_creation_failed");
          }
        }

        auto player = env->CreatePlayer();
        player->SetEventDelegate(std::make_unique<PlayerBridge>(
            binary_messenger_, task_queue_, player.get(),
            main_thread_dispatcher_));
        auto id = player->id();
        auto texture_id = CreateVideoOutput(player.get());
        registry_->players()->InsertPlayer(id, std::move(player));
        shared_result->Success(flutter::EncodableMap(
            {{"player_id", id}, {"texture_id", texture_id}}));
      })) {
    shared_result->Error(kErrorCodePluginTerminated);
  }
}

int64_t MethodChannelHandler::CreateVideoOutput(Player* player) {
  int64_t texture_id = -1;

#ifdef HAVE_FLUTTER_D3D_TEXTURE
  auto outlet = std::make_unique<VideoOutletD3d>(texture_registrar_);
  texture_id = outlet->texture_id();
  auto video_output =
      player->CreateD3D11Output(std::move(outlet), graphics_adapter_);
  player->SetVideoOutput(std::move(video_output));
#else
  auto outlet = std::make_unique<VideoOutlet>(texture_registrar_);
  texture_id = outlet->texture_id();
  auto video_output = player->CreatePixelBufferOutput(std::move(outlet),
                                                      PixelFormat::kFormatRGBA);
  player->SetVideoOutput(std::move(video_output));
#endif

  return texture_id;
}

void MethodChannelHandler::DisposePlayer(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);
  if (auto id = std::get_if<int64_t>(method_call.arguments())) {
    if (!task_queue_->Enqueue(
            [id = *id, shared_result, registry = registry_.get()]() {
              auto player = registry->players()->RemovePlayer(id);
              if (player) {
                shared_result->Success();
              } else {
                shared_result->Error(kErrorCodeInvalidId);
              }
            })) {
      shared_result->Error(kErrorCodePluginTerminated);
    }
  } else {
    shared_result->Error(kErrorCodeInvalidArguments);
  }
}

}  // namespace windows
}  // namespace foxglove
