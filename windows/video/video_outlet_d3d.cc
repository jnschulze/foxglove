#include "video_outlet_d3d.h"

namespace foxglove {
namespace windows {

VideoOutletD3d::VideoOutletD3d(flutter::TextureRegistrar* texture_registrar)
    : TextureOutlet(texture_registrar) {
  texture_ =
      std::make_unique<flutter::TextureVariant>(flutter::GpuSurfaceTexture(
          kFlutterDesktopGpuSurfaceTypeDxgiSharedHandle,
          [this](size_t width,
                 size_t height) -> const FlutterDesktopGpuSurfaceDescriptor* {
            return surface_descriptor();
          }));

  texture_id_ = texture_registrar_->RegisterTexture(texture_.get());
}

void VideoOutletD3d::Present() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (valid()) {
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
  }
}

void VideoOutletD3d::SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (!valid()) {
    return;
  }

  d3d_texture_ = std::move(texture);

  if (!d3d_texture_) {
    surface_descriptor_ = {};
    return;
  }

  D3D11_TEXTURE2D_DESC desc{};
  d3d_texture_->GetDesc(&desc);

  winrt::com_ptr<IDXGIResource1> shared_resource;
  HANDLE handle;
  if (!SUCCEEDED(d3d_texture_->QueryInterface(__uuidof(IDXGIResource1),
                                              shared_resource.put_void())) ||
      !SUCCEEDED(shared_resource->GetSharedHandle(&handle))) {
    std::cerr << "Obtaining texture shared handle failed." << std::endl;
    surface_descriptor_ = {};
  } else {
    surface_descriptor_.handle = handle;
    surface_descriptor_.width = desc.Width;
    surface_descriptor_.height = desc.Height;
    surface_descriptor_.visible_width = desc.Width;
    surface_descriptor_.visible_height = desc.Height;
    surface_descriptor_.release_context = d3d_texture_.get();
    surface_descriptor_.release_callback = [](void* release_context) {
      auto texture = reinterpret_cast<ID3D11Texture2D*>(release_context);
      texture->Release();
    };
  }
  surface_descriptor_.struct_size = sizeof(FlutterDesktopGpuSurfaceDescriptor);
}

const FlutterDesktopGpuSurfaceDescriptor* VideoOutletD3d::surface_descriptor() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (valid()) {
    if (surface_descriptor_.handle && d3d_texture_) {
      d3d_texture_->AddRef();
    }
    return &surface_descriptor_;
  }
  return nullptr;
}

void VideoOutletD3d::Shutdown() { Unregister(); }

VideoOutletD3d::~VideoOutletD3d() { Unregister(); }

}  // namespace windows
}  // namespace foxglove