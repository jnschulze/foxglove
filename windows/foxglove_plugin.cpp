#include "include/foxglove/foxglove_plugin.h"

// This must be included before many other Windows headers.
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>

#include <map>
#include <memory>
#include <sstream>

#include "globals.h"
#include "method_channel_handler.h"

namespace {

class FoxglovePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FoxglovePlugin();

  virtual ~FoxglovePlugin();

 private:
  std::unique_ptr<foxglove::windows::MethodChannelHandler>
      method_channel_handler_;
};

// static
void FoxglovePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "foxglove",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FoxglovePlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->method_channel_handler_->HandleMethodCall(
            call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FoxglovePlugin::FoxglovePlugin()
    : method_channel_handler_(
          std::make_unique<foxglove::windows::MethodChannelHandler>(
              foxglove::g_registry.get())) {}

FoxglovePlugin::~FoxglovePlugin() {}

}  // namespace

void FoxglovePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  FoxglovePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
