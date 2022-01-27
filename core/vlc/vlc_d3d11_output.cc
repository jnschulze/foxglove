#include "vlc/vlc_d3d11_output.h"

#include <vlc/libvlc.h>

#include <iostream>

#include "vlc/vlc_player.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

namespace foxglove {

VlcD3D11Output::VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                               IDXGIAdapter* adapter)
    : delegate_(std::move(delegate)) {
  adapter_.copy_from(adapter);
}

void VlcD3D11Output::Attach(VlcPlayer* player) {
  Initialize();

  libvlc_video_set_output_callbacks(
      player->vlc_player(), libvlc_video_engine_d3d11, SetupCb, CleanupCb,
      ResizeCb, UpdateOutputCb, SwapCb, StartRenderingCb, nullptr, nullptr,
      SelectPlaneCb, this);
}

void VlcD3D11Output::Initialize() {
  IDXGIAdapter* preferred_adapter = adapter_.get();

  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifndef NDEBUG
  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  auto hr = D3D11CreateDevice(
      preferred_adapter,
      (preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
      NULL, creationFlags, NULL, 0, D3D11_SDK_VERSION,
      render_context_.d3d_device.put(), NULL,
      render_context_.d3d_context.put());

  if (!SUCCEEDED(hr)) {
    std::cerr << "D3D11CreateDevice failed " << hr << std::endl;
  }

  ID3D10Multithread* pMultithread;
  hr = render_context_.d3d_device->QueryInterface(__uuidof(ID3D10Multithread),
                                                  (void**)&pMultithread);
  if (SUCCEEDED(hr)) {
    pMultithread->SetMultithreadProtected(TRUE);
    pMultithread->Release();
  }

  D3D11CreateDevice(
      preferred_adapter,
      (preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
      NULL,
      creationFlags |
          D3D11_CREATE_DEVICE_VIDEO_SUPPORT, /* needed for hardware decoding */
      NULL, 0, D3D11_SDK_VERSION, render_context_.d3d_device_vlc.put(), NULL,
      render_context_.d3d_context_vlc.put());
}

void VlcD3D11Output::SetDimensions(VideoDimensions& dimensions) {
  if (current_dimensions_ != dimensions) {
    current_dimensions_ = dimensions;
    dimensions_changed_(dimensions);
  }
}

bool VlcD3D11Output::SetupCb(void** opaque,
                             const libvlc_video_setup_device_cfg_t* cfg,
                             libvlc_video_setup_device_info_t* out) {
  std::cerr << "Setup" << std::endl;

  auto impl = reinterpret_cast<VlcD3D11Output*>(*opaque);

  out->d3d11.device_context = impl->render_context_.d3d_context_vlc.get();
  impl->render_context_.d3d_context_vlc->AddRef();

  return true;
}

void VlcD3D11Output::CleanupCb(void* opaque) {
  std::cerr << "Cleanup !!!" << std::endl;

  auto impl = reinterpret_cast<VlcD3D11Output*>(opaque);
  impl->render_context_.d3d_context_vlc->Release();
}

void VlcD3D11Output::ResizeCb(void* opaque,
                              void (*report_size_change)(void* report_opaque,
                                                         unsigned width,
                                                         unsigned height),
                              void* report_opaque) {
  std::cerr << "RESIZE" << std::endl;

  // if(report_size_change != nullptr) {
  //   report_size_change(report_opaque, 500, 500);
  // }

  // auto impl = reinterpret_cast<D3D11Output::Impl*>(opaque);
  // impl->SendEmptyFrame();
}

bool VlcD3D11Output::UpdateOutputCb(void* opaque,
                                    const libvlc_video_render_cfg_t* cfg,
                                    libvlc_video_output_cfg_t* out) {
  std::cerr << "Update Output" << cfg->width << "x" << cfg->height << std::endl;

  auto impl = reinterpret_cast<VlcD3D11Output*>(opaque);

  // CloseHandle(impl->render_context_.texture_shared_handle);
  // impl->render_context_.texture_shared_handle = nullptr;
  impl->render_context_.texture_vlc = nullptr;
  impl->render_context_.texture_render_target = nullptr;
  impl->render_context_.texture = nullptr;

  impl->SetDimensions(VideoDimensions(cfg->width, cfg->height, 0));

  // DXGI_FORMAT render_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_FORMAT render_format = DXGI_FORMAT_B8G8R8A8_UNORM;

  /* interim texture */
  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.MipLevels = 1;
  texDesc.SampleDesc.Count = 1;
  texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.CPUAccessFlags = 0;
  texDesc.ArraySize = 1;
  texDesc.Format = render_format;
  texDesc.Height = cfg->height;
  texDesc.Width = cfg->width;
  texDesc.MiscFlags =
      D3D11_RESOURCE_MISC_SHARED /*| D3D11_RESOURCE_MISC_SHARED_NTHANDLE*/;

  if (!SUCCEEDED(impl->render_context_.d3d_device->CreateTexture2D(
          &texDesc, NULL, impl->render_context_.texture.put()))) {
    std::cerr << "creating texture failed " << std::endl;
    return false;
  }

  // IDXGIResource1* shared_resource = NULL;
  // impl->render_context_.texture->QueryInterface(__uuidof(IDXGIResource1),
  // (LPVOID*) &shared_resource); auto hr =
  // shared_resource->CreateSharedHandle(NULL,
  // DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE, NULL,
  // &impl->render_context_.texture_shared_handle);

  IDXGIResource* shared_resource = NULL;
  impl->render_context_.texture->QueryInterface(__uuidof(IDXGIResource),
                                                (LPVOID*)&shared_resource);
  auto hr = shared_resource->GetSharedHandle(
      &impl->render_context_.texture_shared_handle);

  shared_resource->Release();

  /*
  ID3D11Device1* d3d11VLC1;
  impl->render_context_.d3d_device_vlc->QueryInterface(__uuidof(ID3D11Device1),
  (LPVOID*) &d3d11VLC1); hr =
  d3d11VLC1->OpenSharedResource1(impl->render_context_.texture_shared_handle,
  __uuidof(ID3D11Texture2D), (void**)&impl->render_context_.texture_vlc);
  d3d11VLC1->Release();
  */

  ID3D11Device* d3d11VLC;
  impl->render_context_.d3d_device_vlc->QueryInterface(__uuidof(ID3D11Device),
                                                       (LPVOID*)&d3d11VLC);
  hr = d3d11VLC->OpenSharedResource(
      impl->render_context_.texture_shared_handle, __uuidof(ID3D11Texture2D),
      impl->render_context_.texture_vlc.put_void());
  d3d11VLC->Release();

  D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};

  renderTargetViewDesc.Format = texDesc.Format;
  renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

  hr = impl->render_context_.d3d_device_vlc->CreateRenderTargetView(
      impl->render_context_.texture_vlc.get(), &renderTargetViewDesc,
      impl->render_context_.texture_render_target.put());
  if (FAILED(hr)) return false;

  ID3D11RenderTargetView* const targets[1] = {
      impl->render_context_.texture_render_target.get()};
  impl->render_context_.d3d_context_vlc->OMSetRenderTargets(1, targets, NULL);

  out->dxgi_format = render_format;
  out->full_range = true;
  out->colorspace = libvlc_video_colorspace_BT709;
  out->primaries = libvlc_video_primaries_BT709;
  out->transfer = libvlc_video_transfer_func_SRGB;

  impl->delegate_->SetTexture(impl->render_context_.texture);

  return true;
}

void VlcD3D11Output::SwapCb(void* opaque) {
  // std::cerr << "Swap" << std::endl;

  auto impl = reinterpret_cast<VlcD3D11Output*>(opaque);
  impl->delegate_->Present();
}

bool VlcD3D11Output::StartRenderingCb(void* opaque, bool enter) {
  // std::cerr << "Start rendering " << enter << std::endl;

  /*
  auto impl = reinterpret_cast<D3D11Output::Impl*>(opaque);
  static const FLOAT black_bgra[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  impl->render_context_.d3d_context_vlc->ClearRenderTargetView(
      impl->render_context_.texture_render_target.get(), black_bgra);
  */

  return true;
}

bool VlcD3D11Output::SelectPlaneCb(void* opaque, size_t plane, void* out) {
  ID3D11RenderTargetView** output = static_cast<ID3D11RenderTargetView**>(out);
  auto impl = reinterpret_cast<VlcD3D11Output*>(opaque);

  // we only support one packed RGBA plane
  if (plane != 0) {
    return false;
  }

  // we don't really need to return it as we already do the
  // OMSetRenderTargets().
  *output = impl->render_context_.texture_render_target.get();
  return true;
}

}  // namespace foxglove
