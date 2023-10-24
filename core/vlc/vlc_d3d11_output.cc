#include "vlc/vlc_d3d11_output.h"

#include <cassert>

#include "vlc/vlc_player.h"

namespace foxglove {

VlcD3D11Output::VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                               winrt::com_ptr<IDXGIAdapter> adapter)
    : delegate_(std::move(delegate)), adapter_(std::move(adapter)) {}

VlcD3D11Output::~VlcD3D11Output() {
  const std::lock_guard<std::mutex> lock(render_context_mutex_);
}

void VlcD3D11Output::Attach(VlcPlayer* player) {
  if (Initialize()) {
    libvlc_video_set_output_callbacks(
        player->vlc_player(), libvlc_video_engine_d3d11, SetupCb, CleanupCb,
        ResizeCb, UpdateOutputCb, SwapCb, StartRenderingCb, nullptr, nullptr,
        SelectPlaneCb, this);
  }
}

bool VlcD3D11Output::Initialize() {
  const std::lock_guard lock(render_context_mutex_);
  return render_context_.Initialize(adapter_.get());
}

bool VlcD3D11Output::SetupCb(void** opaque,
                             const libvlc_video_setup_device_cfg_t* cfg,
                             libvlc_video_setup_device_info_t* out) {
  const auto self = static_cast<VlcD3D11Output*>(*opaque);

  const std::lock_guard lock(self->render_context_mutex_);
  auto d3d_context_vlc = self->render_context_.d3d_device_context_vlc();
  assert(d3d_context_vlc);
  d3d_context_vlc->AddRef();
  out->d3d11.device_context = d3d_context_vlc;
  return true;
}

void VlcD3D11Output::CleanupCb(void* opaque) {
  const auto self = static_cast<VlcD3D11Output*>(opaque);
  const std::lock_guard lock(self->render_context_mutex_);

  auto d3d_context_vlc = self->render_context_.d3d_device_context_vlc();
  if (d3d_context_vlc) {
    d3d_context_vlc->Release();
  }
}

void VlcD3D11Output::ResizeCb(
    void* opaque, libvlc_video_output_resize_cb report_size_change,
    // libvlc_video_output_mouse_move_cb report_mouse_move,
    // libvlc_video_output_mouse_press_cb report_mouse_pressed,
    // libvlc_video_output_mouse_release_cb report_mouse_released,
    void* report_opaque) {}

bool VlcD3D11Output::UpdateOutputCb(void* opaque,
                                    const libvlc_video_render_cfg_t* cfg,
                                    libvlc_video_output_cfg_t* out) {
  constexpr DXGI_FORMAT kRenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

  const auto self = static_cast<VlcD3D11Output*>(opaque);
  const std::lock_guard lock(self->render_context_mutex_);

  // 0 dimensions are not allowed.
  const auto width = cfg->width == 0 ? 8 : cfg->width;
  const auto height = cfg->height == 0 ? 8 : cfg->height;

  self->delegate_->SetTexture(nullptr);

  const auto render_context = &self->render_context_;
  if (!render_context->Update(width, height, kRenderFormat)) {
    self->SetDimensions(VideoDimensions(0, 0, 0));
    return false;
  }

  memset(out, 0, sizeof(libvlc_video_output_cfg_t));
  out->colorspace = libvlc_video_colorspace_BT709;
  out->dxgi_format = kRenderFormat;
  out->full_range = true;
  out->orientation = libvlc_video_orient_top_left;
  out->primaries = libvlc_video_primaries_BT709;
  out->transfer = libvlc_video_transfer_func_SRGB;

  winrt::com_ptr<ID3D11Texture2D> texture;
  texture.copy_from(render_context->texture());
  self->delegate_->SetTexture(std::move(texture));
  self->SetDimensions(VideoDimensions(width, height, 0));
  return true;
}

void VlcD3D11Output::SwapCb(void* opaque) {
  const auto self = static_cast<VlcD3D11Output*>(opaque);
  self->delegate_->Present();
}

bool VlcD3D11Output::StartRenderingCb(void* opaque, bool enter) { return true; }

bool VlcD3D11Output::SelectPlaneCb(void* opaque, size_t plane, void* out) {
  ID3D11RenderTargetView** output = static_cast<ID3D11RenderTargetView**>(out);
  const auto self = static_cast<VlcD3D11Output*>(opaque);

  // we only support one packed RGBA plane
  if (plane != 0) {
    return false;
  }

  {
    const std::lock_guard lock(self->render_context_mutex_);
    // we don't really need to return it as we already do the
    // OMSetRenderTargets().
    *output = self->render_context_.render_target();
  }

  return true;
}

}  // namespace foxglove
