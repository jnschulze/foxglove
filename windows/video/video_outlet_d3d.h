#pragma once

#include <mutex>

#include "texture_registry.h"
#include "video/d3d11_output.h"
#include "video/video_output.h"

namespace foxglove {
namespace windows {

class VideoOutletD3dState {
 public:
  VideoOutletD3dState(TextureRegistry* texture_registry);
  ~VideoOutletD3dState();
  void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture);

  inline TextureRegistration* registration() const {
    return texture_registration_.get();
  }

 private:
  std::mutex mutex_;
  std::unique_ptr<flutter::TextureVariant> texture_;
  std::unique_ptr<TextureRegistration> texture_registration_;
  HANDLE shared_handle_ = INVALID_HANDLE_VALUE;
  winrt::com_ptr<ID3D11Texture2D> d3d_texture_;
  FlutterDesktopGpuSurfaceDescriptor surface_descriptor_{};
  const FlutterDesktopGpuSurfaceDescriptor* surface_descriptor();

  inline bool is_valid() const { return texture_registration_->is_valid(); }
};

class VideoOutletD3d : public D3D11OutputDelegate {
 public:
  VideoOutletD3d(TextureRegistry* texture_registry);
  virtual ~VideoOutletD3d();

  void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) override;
  void Present() override;
  inline int64_t texture_id() const {
    return state_->registration()->texture_id();
  }

 private:
  std::shared_ptr<VideoOutletD3dState> state_;
};

}  // namespace windows
}  // namespace foxglove
