#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <mutex>

namespace foxglove {
namespace windows {
class TextureOutlet {
 public:
  int64_t texture_id() const { return texture_id_; }

 protected:
  std::mutex mutex_;
  bool is_unregistering_ = false;
  int64_t texture_id_ = -1;
  flutter::TextureRegistrar* texture_registrar_ = nullptr;
  TextureOutlet(flutter::TextureRegistrar* texture_registrar);
  inline bool valid() const { return !is_unregistering_; }

  // Unregisters a texture and synchronously waits for its completion.
  //
  // Note:
  // This function synchronously waits for a callback that gets invoked from the Flutter raster thread.
  // Be careful to avoid deadlocks.
  void UnregisterTexture(int64_t texture_id);

  void Unregister();
};

}  // namespace windows
}  // namespace foxglove
