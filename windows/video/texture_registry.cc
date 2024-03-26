#include "texture_registry.h"

#include <cassert>

namespace foxglove {
namespace windows {

TextureRegistry::TextureRegistry(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar) {}

std::unique_ptr<TextureRegistration> TextureRegistry::RegisterTexture(
    flutter::TextureVariant* texture) {
  auto id = texture_registrar_->RegisterTexture(texture);
  return std::make_unique<TextureRegistration>(id, this);
}

TextureRegistration::TextureRegistration(int64_t texture_id,
                                         TextureRegistry* registry)
    : texture_id_(texture_id),
      registry_(registry),
      state_(TextureRegistrationState::kRegistered) {}

void TextureRegistration::MarkFrameAvailable() const {
  if (is_valid()) {
    registry_->registrar()->MarkTextureFrameAvailable(texture_id_);
  }
}

void TextureRegistration::Unregister(std::function<void()> callback) {
  if (state_.exchange(TextureRegistrationState::kUnregistering) !=
      TextureRegistrationState::kRegistered) {
    return;
  }

  registry_->registrar()->UnregisterTexture(
      texture_id_, [&, callback = std::move(callback)]() {
        assert(state_ == TextureRegistrationState::kUnregistering);
        state_ = TextureRegistrationState::kUnregistered;
        if (callback) {
          callback();
        }
      });
}

}  // namespace windows
}  // namespace foxglove
