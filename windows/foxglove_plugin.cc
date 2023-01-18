#include "foxglove_plugin.h"

#include <iostream>
#include <sstream>

#include "globals.h"
#include "plugin_state.h"

namespace foxglove {
namespace windows {

// static
void FoxglovePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto plugin = std::make_unique<FoxglovePlugin>(registrar->messenger(),
                                                 registrar->texture_registrar(),
                                                 registrar->GetView());
  registrar->AddPlugin(std::move(plugin));
}

FoxglovePlugin::FoxglovePlugin(flutter::BinaryMessenger* binary_messenger,
                               flutter::TextureRegistrar* texture_registrar,
                               flutter::FlutterView* view) {
  winrt::com_ptr<IDXGIAdapter> graphics_adapter;
  graphics_adapter.copy_from(view->GetGraphicsAdapter());
  if (graphics_adapter) {
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(graphics_adapter->GetDesc(&desc))) {
      std::wcerr << "Graphics adapter: " << desc.Description << std::endl;
    }
  }

  method_channel_handler_ =
      std::make_unique<foxglove::windows::MethodChannelHandler>(
          foxglove::g_registry.get(), binary_messenger, texture_registrar,
          std::move(graphics_adapter));

  method_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          binary_messenger, "foxglove",
          &flutter::StandardMethodCodec::GetInstance());

  method_channel_->SetMethodCallHandler([this](const auto& call, auto result) {
    method_channel_handler_->HandleMethodCall(call, std::move(result));
  });
}

FoxglovePlugin::~FoxglovePlugin() {
  // Channel handlers must not be unset in plugin destructors (see
  // https://github.com/flutter/flutter/issues/118611)
  // method_channel_->SetMethodCallHandler(nullptr);
  PluginState::SetIsTerminating();

  foxglove::g_registry->players()->Clear();
  foxglove::g_registry->environments()->Clear();
}

}  // namespace windows

}  // namespace foxglove
