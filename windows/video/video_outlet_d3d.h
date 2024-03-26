#pragma once

#include <mutex>

#include "texture_registry.h"
#include "video/d3d11_output.h"
#include "video/video_output.h"
#include "video_outlet_base.h"

namespace foxglove {
namespace windows {

class VideoOutletD3dState : public VideoOutletBase {
 public:
  VideoOutletD3dState(TextureRegistry* texture_registry);
  ~VideoOutletD3dState();
  void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture);

 private:
  std::mutex mutex_;
  HANDLE shared_handle_ = INVALID_HANDLE_VALUE;
  winrt::com_ptr<ID3D11Texture2D> d3d_texture_;
  FlutterDesktopGpuSurfaceDescriptor surface_descriptor_{};
  const FlutterDesktopGpuSurfaceDescriptor* surface_descriptor();
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
