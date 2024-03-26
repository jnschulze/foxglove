#pragma once

#include <flutter/plugin_registrar_windows.h>

#include <atomic>
#include <memory>

namespace foxglove {
namespace windows {

enum class TextureRegistrationState {
  kRegistered,
  kUnregistering,
  kUnregistered
};

class TextureRegistry;

class TextureRegistration {
 public:
  TextureRegistration(int64_t texture_id, TextureRegistry* registry);

  inline int64_t texture_id() const { return texture_id_; }
  inline TextureRegistrationState state() const { return state_; }
  inline bool is_valid() const {
    return state_ == TextureRegistrationState::kRegistered;
  }

  void MarkFrameAvailable() const;

  // Unregisters the texture and invokes |callback| upon completion.
  // The callback gets invoked on the Flutter raster thread.
  void Unregister(std::function<void()> callback);

 private:
  TextureRegistry* registry_ = nullptr;
  std::atomic<TextureRegistrationState> state_;
  int64_t texture_id_ = -1;
};

class TextureRegistry {
 public:
  TextureRegistry(flutter::TextureRegistrar* texture_registrar);

  std::unique_ptr<TextureRegistration> RegisterTexture(
      flutter::TextureVariant* texture);

  flutter::TextureRegistrar* registrar() const { return texture_registrar_; }

 private:
  flutter::TextureRegistrar* texture_registrar_ = nullptr;
};

}  // namespace windows
}  // namespace foxglove
