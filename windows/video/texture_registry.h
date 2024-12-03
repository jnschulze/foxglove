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
  static constexpr auto kInvalidTextureId = -1;

  TextureRegistration(int64_t texture_id, TextureRegistry* registry);
  TextureRegistration();

  inline int64_t texture_id() const { return texture_id_; }
  inline TextureRegistrationState state() const { return state_; }
  inline bool is_valid() const {
    return state_ == TextureRegistrationState::kRegistered;
  }

  void MarkFrameAvailable() const;

  // Unregisters the texture and invokes |callback| upon completion.
  // The callback gets invoked on the Flutter raster thread.
  bool Unregister(std::function<void()> callback);

 private:
  const TextureRegistry* registry_ = nullptr;
  const int64_t texture_id_;
  std::atomic<TextureRegistrationState> state_;
};

class TextureRegistry {
 public:
  TextureRegistry(flutter::TextureRegistrar* texture_registrar);

  std::unique_ptr<TextureRegistration> RegisterTexture(
      flutter::TextureVariant* texture);

  void MarkTextureFrameAvailable(int64_t texture_id) const;
  void UnregisterTexture(int64_t texture_id,
                         std::function<void()> callback) const;
  void Invalidate();

 private:
  flutter::TextureRegistrar* texture_registrar_ = nullptr;
};

}  // namespace windows
}  // namespace foxglove
