#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>

#include "method_channel_handler.h"

namespace foxglove {
namespace windows {
class FoxglovePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  FoxglovePlugin(flutter::BinaryMessenger* binary_messenger,
                 flutter::TextureRegistrar* texture_registrar,
                 flutter::FlutterView* view);

  virtual ~FoxglovePlugin();

  // Disallow copy and assign.
  FoxglovePlugin(const FoxglovePlugin&) = delete;
  FoxglovePlugin& operator=(const FoxglovePlugin&) = delete;

 private:
  std::unique_ptr<MethodChannelHandler> method_channel_handler_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;
};
}  // namespace windows
}  // namespace foxglove