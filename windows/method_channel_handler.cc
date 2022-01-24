#include "method_channel_handler.h"

#include "player_environment.h"
#include "vlc/vlc_environment.h"

namespace foxglove {
namespace windows {

namespace {
constexpr auto kMethodCreateEnvironment = "createEnvironment";
constexpr auto kMethodDisposeEnvironment = "disposeEnvironment";
constexpr auto kMethodCreatePlayer = "createPlayer";
constexpr auto kMethodDisposePlayer = "disposePlayer";

constexpr auto kErrorCodeInvalidArguments = "invalid_args";
constexpr auto kErrorCodeInvalidId = "invalid_id";
}  // namespace

MethodChannelHandler::MethodChannelHandler(ObjectRegistry *object_registry)
    : registry_(object_registry), task_queue_(std::make_unique<TaskQueue>()) {}

void MethodChannelHandler::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare(kMethodCreateEnvironment) == 0) {
    return CreateEnvironment(method_call, std::move(result));
  }

  if (method_call.method_name().compare(kMethodDisposeEnvironment) == 0) {
    return DisposeEnvironment(method_call, std::move(result));
  }

  if (method_call.method_name().compare(kMethodCreatePlayer) == 0) {
    return CreatePlayer(method_call, std::move(result));
  }

  if (method_call.method_name().compare(kMethodDisposePlayer) == 0) {
    return DisposePlayer(method_call, std::move(result));
  }

  result->NotImplemented();
}

void MethodChannelHandler::CreateEnvironment(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::vector<std::string> arguments;
  if (auto args = std::get_if<flutter::EncodableMap>(method_call.arguments())) {
    auto it = args->find(flutter::EncodableValue("args"));
    if (it != args->end()) {
      if (auto arglist = std::get_if<flutter::EncodableList>(&it->second)) {
        for (const auto &arg : *arglist) {
          if (auto str = std::get_if<std::string>(&arg)) {
            arguments.push_back(*str);
          }
        }
      }
    }
  }

  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

  task_queue_->Enqueue([args = std::move(arguments), shared_result,
                        registry = registry_]() {
    auto env = std::make_shared<foxglove::VlcEnvironment>(args);
    registry->environments()->RegisterEnvironment(env->id(), std::move(env));
    shared_result->Success(env->id());
  });
}

void MethodChannelHandler::DisposeEnvironment(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (auto id = std::get_if<int64_t>(method_call.arguments())) {
    std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
        shared_result = std::move(result);

    task_queue_->Enqueue([id = *id, shared_result, registry = registry_]() {
      if (registry->environments()->RemoveEnvironment(id)) {
        shared_result->Success();
      } else {
        shared_result->Error(kErrorCodeInvalidId);
      }
    });
  } else {
    result->Error("invalid_arguments");
  }
}

void MethodChannelHandler::CreatePlayer(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  std::optional<int64_t> environment_id;
  if (auto args = std::get_if<flutter::EncodableMap>(method_call.arguments())) {
    auto it = args->find(flutter::EncodableValue("environmentId"));
    if (it != args->end()) {
      if (auto id = std::get_if<int64_t>(&it->second)) {
        environment_id = *id;
      }
    }
  }

  std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
      shared_result = std::move(result);

  task_queue_->Enqueue([environment_id, shared_result, registry = registry_]() {
    std::shared_ptr<foxglove::PlayerEnvironment> env;
    if (environment_id.has_value()) {
      env = registry->environments()->GetEnvironment(environment_id.value());
      if (!env) {
        return shared_result->Error(kErrorCodeInvalidId);
      }
    } else {
      std::vector<std::string> args{};
      env = std::make_shared<foxglove::VlcEnvironment>(args);
      if (!env) {
        return shared_result->Error("env_creation_failed");
      }
    }

    auto player = env->CreatePlayer();
    auto id = player->id();
    registry->players()->InsertPlayer(player->id(), std::move(player));
    shared_result->Success(flutter::EncodableMap(

        {{"player_id", id}}));
  });
}

void MethodChannelHandler::DisposePlayer(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (auto id = std::get_if<int64_t>(method_call.arguments())) {
    std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>
        shared_result = std::move(result);

    task_queue_->Enqueue([id = *id, shared_result, registry = registry_]() {
      if (registry->players()->RemovePlayer(id)) {
        shared_result->Success();
      } else {
        shared_result->Error(kErrorCodeInvalidId);
      }
    });
  } else {
    result->Error(kErrorCodeInvalidArguments);
  }
}

}  // namespace windows
}  // namespace foxglove