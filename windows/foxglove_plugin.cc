#include "foxglove_plugin.h"

#include "base/logging.h"
#include "base/string_utils.h"
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
  SetupLogging();

  winrt::com_ptr<IDXGIAdapter> graphics_adapter;
  graphics_adapter.attach(view->GetGraphicsAdapter());
  if (graphics_adapter) {
    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(graphics_adapter->GetDesc(&desc))) {
      LOG(INFO) << "Graphics adapter: " << util::Utf8FromUtf16(desc.Description)
                << std::endl;
    }
  }

  method_channel_handler_ =
      std::make_unique<foxglove::windows::MethodChannelHandler>(
          std::make_unique<PlayerRegistry>(), binary_messenger,
          texture_registrar, std::move(graphics_adapter));

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

  method_channel_handler_->Terminate();
}

}  // namespace windows
}  // namespace foxglove
