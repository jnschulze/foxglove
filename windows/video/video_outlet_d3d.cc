#include "video_outlet_d3d.h"

namespace foxglove {
namespace windows {

VideoOutletD3d::VideoOutletD3d(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar) {
  texture_ =
      std::make_unique<flutter::TextureVariant>(flutter::GpuSurfaceTexture(
          kFlutterDesktopGpuSurfaceTypeDxgi,
          [this](size_t width,
                 size_t height) -> const FlutterDesktopGpuSurfaceDescriptor* {
            return &surface_descriptor_;
          }));

  texture_id_ = texture_registrar_->RegisterTexture(texture_.get());
}

void VideoOutletD3d::Present() {
  texture_registrar_->MarkTextureFrameAvailable(texture_id_);
}

void VideoOutletD3d::SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) {
  std::cerr << "SET TEXTURE" << std::endl;

  if (!texture) {
    surface_descriptor_ = {};
    return;
  }

  D3D11_TEXTURE2D_DESC desc{};
  texture->GetDesc(&desc);

  winrt::com_ptr<IDXGIResource1> shared_resource;
  HANDLE handle;
  if (!SUCCEEDED(texture->QueryInterface(__uuidof(IDXGIResource1),
                                         (LPVOID*)shared_resource.put())) ||
      !SUCCEEDED(shared_resource->GetSharedHandle(&handle))) {
    std::cerr << "Obtaining texture shared handle failed." << std::endl;
    surface_descriptor_ = {};
  } else {
    surface_descriptor_.handle = handle;
    surface_descriptor_.width = desc.Width;
    surface_descriptor_.height = desc.Height;
    surface_descriptor_.visible_width = desc.Width;
    surface_descriptor_.visible_height = desc.Height;
  }
}

VideoOutletD3d::~VideoOutletD3d() {
  texture_registrar_->UnregisterTexture(texture_id_);
}

}  // namespace windows
}  // namespace foxglove