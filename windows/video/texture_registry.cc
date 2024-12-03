#include "texture_registry.h"

#include <cassert>

#include "base/logging.h"

namespace foxglove {
namespace windows {

TextureRegistry::TextureRegistry(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar) {}

std::unique_ptr<TextureRegistration> TextureRegistry::RegisterTexture(
    flutter::TextureVariant* texture) {
  if (texture_registrar_) {
    auto id = texture_registrar_->RegisterTexture(texture);
    LOG(LOG_TRACE) << "Registered texture with ID " << id << std::endl;
    return std::make_unique<TextureRegistration>(id, this);
  }
  return std::make_unique<TextureRegistration>();
}

void TextureRegistry::MarkTextureFrameAvailable(int64_t id) const {
  if (texture_registrar_) {
    texture_registrar_->MarkTextureFrameAvailable(id);
  }
}

void TextureRegistry::UnregisterTexture(int64_t texture_id,
                                        std::function<void()> callback) const {
  if (texture_registrar_) {
    texture_registrar_->UnregisterTexture(texture_id, std::move(callback));
  } else {
    callback();
  }
}

void TextureRegistry::Invalidate() {
  LOG(LOG_TRACE) << "Invalidating texture registry" << std::endl;
  texture_registrar_ = nullptr;
}

TextureRegistration::TextureRegistration(int64_t texture_id,
                                         TextureRegistry* registry)
    : registry_(registry),
      state_(TextureRegistrationState::kRegistered),
      texture_id_(texture_id) {}

TextureRegistration::TextureRegistration()
    : registry_(nullptr),
      state_(TextureRegistrationState::kUnregistered),
      texture_id_(kInvalidTextureId) {}

void TextureRegistration::MarkFrameAvailable() const {
  if (is_valid() && registry_) {
    registry_->MarkTextureFrameAvailable(texture_id_);
  }
}

bool TextureRegistration::Unregister(std::function<void()> callback) {
  if (!registry_ || state_.exchange(TextureRegistrationState::kUnregistering) !=
                        TextureRegistrationState::kRegistered) {
    LOG(LOG_TRACE) << "Skipping texture unregistration" << std::endl;
    return false;
  }

  LOG(LOG_TRACE) << "Attempting to unregister texture" << std::endl;

  registry_->UnregisterTexture(
      texture_id_, [&, callback = std::move(callback)]() {
        assert(state_ == TextureRegistrationState::kUnregistering);
        state_ = TextureRegistrationState::kUnregistered;
        LOG(LOG_TRACE) << "Texture unregistered." << std::endl;
        if (callback) {
          callback();
        }
      });

  return true;
}

}  // namespace windows
}  // namespace foxglove
