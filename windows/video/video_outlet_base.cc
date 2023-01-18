#include "video_outlet_base.h"

#include <future>

namespace foxglove {
namespace windows {

TextureOutlet::TextureOutlet(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar) {}

// Unregisters a texture and synchronously waits for its completion.
void TextureOutlet::UnregisterTexture(int64_t texture_id) {
  std::promise<void> promise;
  texture_registrar_->UnregisterTexture(texture_id,
                                        [&]() { promise.set_value(); });
  promise.get_future().wait();
}

void TextureOutlet::Unregister() {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (is_unregistering_) {
      return;
    }
    is_unregistering_ = true;
    UnregisterTexture(texture_id_);
  }
}

}  // namespace windows
}  // namespace foxglove