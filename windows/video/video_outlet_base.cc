#include "video_outlet_base.h"

#include <chrono>
#include <future>
#include <iostream>

namespace foxglove {
namespace windows {

using namespace std::chrono_literals;

TextureOutlet::TextureOutlet(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar) {}

void TextureOutlet::UnregisterTexture(int64_t texture_id) {
  std::promise<void> promise;
  texture_registrar_->UnregisterTexture(texture_id,
                                        [&]() { promise.set_value(); });
  if (promise.get_future().wait_for(5s) == std::future_status::timeout) {
    std::cerr << "Waiting for texture unregistration timed out. Deadlock?"
              << std::endl;
  }
}

void TextureOutlet::Unregister() {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (is_unregistering_) {
      return;
    }
    is_unregistering_ = true;
  }
  UnregisterTexture(texture_id_);
}

}  // namespace windows
}  // namespace foxglove