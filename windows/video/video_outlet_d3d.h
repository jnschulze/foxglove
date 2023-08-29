#pragma once

#include "video/d3d11_output.h"
#include "video/video_output.h"
#include "video_outlet_base.h"

namespace foxglove {
namespace windows {

class VideoOutletD3d : public TextureOutlet, public D3D11OutputDelegate {
 public:
  VideoOutletD3d(flutter::TextureRegistrar* texture_registrar);
  virtual ~VideoOutletD3d();

  void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) override;
  void Present() override;

 private:
  HANDLE shared_handle_ = INVALID_HANDLE_VALUE;
  std::unique_ptr<flutter::TextureVariant> texture_ = nullptr;
  winrt::com_ptr<ID3D11Texture2D> d3d_texture_;
  FlutterDesktopGpuSurfaceDescriptor surface_descriptor_{};
  const FlutterDesktopGpuSurfaceDescriptor* surface_descriptor();
};

}  // namespace windows

}  // namespace foxglove