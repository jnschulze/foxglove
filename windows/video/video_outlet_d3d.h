#pragma once

#include "video/d3d11_output.h"
#include "video/video_output.h"
#include "video_outlet_base.h"

namespace foxglove {
namespace windows {

class VideoOutletD3d : public TextureOutlet, public D3D11OutputDelegate {
 public:
  VideoOutletD3d(flutter::TextureRegistrar* texture_registrar);
  int64_t texture_id() const override { return texture_id_; }
  void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) override;
  void Present() override;
  virtual ~VideoOutletD3d();

 private:
  flutter::TextureRegistrar* texture_registrar_ = nullptr;
  std::unique_ptr<flutter::TextureVariant> texture_ = nullptr;
  int64_t texture_id_;

  FlutterDesktopGpuSurfaceDescriptor surface_descriptor_{};
};

}  // namespace windows

}  // namespace foxglove