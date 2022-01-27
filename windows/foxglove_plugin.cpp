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
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  FoxglovePlugin(flutter::BinaryMessenger* binary_messenger,
                 flutter::TextureRegistrar* texture_registrar,
                 flutter::FlutterView* view);

  virtual ~FoxglovePlugin();

 private:
  std::unique_ptr<foxglove::windows::MethodChannelHandler>
      method_channel_handler_;
};

// static
void FoxglovePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "foxglove",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FoxglovePlugin>(registrar->messenger(),
                                                 registrar->texture_registrar(),
                                                 registrar->GetView());

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->method_channel_handler_->HandleMethodCall(
            call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FoxglovePlugin::FoxglovePlugin(flutter::BinaryMessenger* binary_messenger,
                               flutter::TextureRegistrar* texture_registrar,
                               flutter::FlutterView* view) {
  winrt::com_ptr<IDXGIAdapter> graphics_adapter;
#ifdef HAVE_FLUTTER_D3D_TEXTURE
  if (view->GetGraphicsAdapter(graphics_adapter.put())) {
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(graphics_adapter_->GetDesc(&desc))) {
      std::wcerr << "Graphics adapter: " << desc.Description << std::endl;
    }
  }
#endif

  method_channel_handler_ =
      std::make_unique<foxglove::windows::MethodChannelHandler>(
          foxglove::g_registry.get(), binary_messenger, texture_registrar,
          std::move(graphics_adapter));
}

FoxglovePlugin::~FoxglovePlugin() {
  foxglove::g_registry->environments()->Clear();
}

}  // namespace

void FoxglovePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  FoxglovePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
