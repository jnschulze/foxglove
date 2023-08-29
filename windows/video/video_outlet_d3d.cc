#include "video_outlet_d3d.h"

#include <iostream>

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
  const std::lock_guard lock(mutex_);
  if (valid()) {
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
  }
}

void VideoOutletD3d::SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) {
  const std::lock_guard lock(mutex_);

  const HANDLE previous_handle = shared_handle_;

  d3d_texture_ = std::move(texture);

  surface_descriptor_ = {};
  surface_descriptor_.struct_size = sizeof(FlutterDesktopGpuSurfaceDescriptor);

  if (valid() && d3d_texture_) {
    winrt::com_ptr<IDXGIResource1> shared_resource;
    if (SUCCEEDED(d3d_texture_->QueryInterface(__uuidof(IDXGIResource1),
                                               shared_resource.put_void())) &&
        SUCCEEDED(shared_resource->CreateSharedHandle(
            NULL, DXGI_SHARED_RESOURCE_READ, NULL, &shared_handle_))) {
      D3D11_TEXTURE2D_DESC desc{};
      d3d_texture_->GetDesc(&desc);
      surface_descriptor_.handle = shared_handle_;
      surface_descriptor_.width = desc.Width;
      surface_descriptor_.height = desc.Height;
      surface_descriptor_.visible_width = desc.Width;
      surface_descriptor_.visible_height = desc.Height;
      surface_descriptor_.release_context = this;
      surface_descriptor_.release_callback = [](void* release_context) {
        const auto self = static_cast<VideoOutletD3d*>(release_context);
        // Unlocks mutex locked in surface_descriptor()
        self->d3d_texture_->Release();
        self->mutex_.unlock();
      };
    } else {
      std::cerr << "Obtaining texture shared handle failed." << std::endl;
    }
  }

  // Close the previous texture's HANDLE after creating a HANDLE for the new
  // texture to ensure that IDXGIResource1::CreateSharedHandle won't reuse the
  // previous HANDLE number as this will lead to problems due do an optimization
  // in the Flutter engine:
  // https://github.com/flutter/engine/blob/7bf0d33afe6282bbe41def1a271e05c721d6b396/shell/platform/windows/external_texture_d3d.cc#L84C4-L84C4
  if (previous_handle != INVALID_HANDLE_VALUE &&
      shared_handle_ != previous_handle) {
    CloseHandle(previous_handle);
  }
}

const FlutterDesktopGpuSurfaceDescriptor* VideoOutletD3d::surface_descriptor() {
  std::unique_lock lock(mutex_);
  if (valid() && surface_descriptor_.handle) {
    d3d_texture_->AddRef();

    // Releases unique_lock without unlocking mutex (gets unlocked in
    // release_callback)
    lock.release();
    return &surface_descriptor_;
  }
  return nullptr;
}

VideoOutletD3d::~VideoOutletD3d() {
  Unregister();

  if (shared_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(shared_handle_);
    shared_handle_ = INVALID_HANDLE_VALUE;
  }
}

}  // namespace windows
}  // namespace foxglove
