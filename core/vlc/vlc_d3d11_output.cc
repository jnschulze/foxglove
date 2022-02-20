#include "vlc/vlc_d3d11_output.h"

#include <vlc/libvlc.h>

#include <cassert>
#include <iostream>

#include "vlc/vlc_player.h"

#ifdef DEBUG_D3D11_LEAKS
#include <dxgidebug.h>
#include <initguid.h>
#endif

static void list_dxgi_leaks() {
#ifdef DEBUG_D3D11_LEAKS
  HMODULE dxgidebug_dll = LoadLibrary(TEXT("DXGIDEBUG.DLL"));
  if (dxgidebug_dll) {
    typedef HRESULT(WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void**);
    LPDXGIGETDEBUGINTERFACE pf_DXGIGetDebugInterface;
    pf_DXGIGetDebugInterface =
        reinterpret_cast<LPDXGIGETDEBUGINTERFACE>(reinterpret_cast<void*>(
            GetProcAddress(dxgidebug_dll, "DXGIGetDebugInterface")));
    if (pf_DXGIGetDebugInterface) {
      IDXGIDebug* pDXGIDebug;
      if (SUCCEEDED(pf_DXGIGetDebugInterface(__uuidof(IDXGIDebug),
                                             (void**)&pDXGIDebug)))
        pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
      pDXGIDebug->Release();
    }
    FreeLibrary(dxgidebug_dll);
  }
#endif  // DEBUG_D3D11_LEAKS
}

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

namespace foxglove {

VlcD3D11Output::VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                               IDXGIAdapter* adapter)
    : delegate_(std::move(delegate)) {
  adapter_.copy_from(adapter);
}

VlcD3D11Output::~VlcD3D11Output() {
#ifdef DEBUG_D3D11_LEAKS
  list_dxgi_leaks();
#endif
}

void VlcD3D11Output::Attach(VlcPlayer* player) {
  Initialize();

  libvlc_video_set_output_callbacks(
      player->vlc_player(), libvlc_video_engine_d3d11, SetupCb, CleanupCb,
      ResizeCb, UpdateOutputCb, SwapCb, StartRenderingCb, nullptr, nullptr,
      SelectPlaneCb, this);
}

void VlcD3D11Output::Shutdown() {
  delegate_->Shutdown();

  // libvlc_video_set_output_callbacks(
  //     player_->vlc_player(), libvlc_video_engine_disable, nullptr, nullptr,
  //     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  //     nullptr);
}

bool VlcD3D11Output::Initialize() {
  auto render_context = &render_context_;
  IDXGIAdapter* preferred_adapter = adapter_.get();

  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifndef NDEBUG
  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  auto hr = D3D11CreateDevice(
      preferred_adapter,
      (preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
      NULL, creationFlags, NULL, 0, D3D11_SDK_VERSION,
      render_context->d3d_device.put(), NULL,
      render_context->d3d_context.put());

  if (FAILED(hr)) {
    std::cerr << "D3D11CreateDevice failed: " << hr << std::endl;
    return false;
  }

  ID3D10Multithread* pMultithread;
  hr = render_context->d3d_device->QueryInterface(__uuidof(ID3D10Multithread),
                                                  (void**)&pMultithread);
  if (SUCCEEDED(hr)) {
    pMultithread->SetMultithreadProtected(TRUE);
    pMultithread->Release();
  }

  hr = D3D11CreateDevice(
      preferred_adapter,
      (preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
      NULL,
      creationFlags |
          D3D11_CREATE_DEVICE_VIDEO_SUPPORT, /* needed for hardware decoding */
      NULL, 0, D3D11_SDK_VERSION, render_context->d3d_device_vlc.put(), NULL,
      render_context->d3d_context_vlc.put());
  if (FAILED(hr)) {
    std::cerr << "D3D11CreateDevice failed: " << hr << std::endl;
    return false;
  }

  return true;
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
  auto self = reinterpret_cast<VlcD3D11Output*>(*opaque);
  out->d3d11.device_context = self->render_context_.d3d_context_vlc.get();
  self->render_context_.d3d_context_vlc->AddRef();

  return true;
}

void VlcD3D11Output::CleanupCb(void* opaque) {
  auto self = reinterpret_cast<VlcD3D11Output*>(opaque);
  self->render_context_.d3d_context_vlc->AddRef();
}

void VlcD3D11Output::ResizeCb(void* opaque,
                              void (*report_size_change)(void* report_opaque,
                                                         unsigned width,
                                                         unsigned height),
                              void* report_opaque) {}

// Static
void VlcD3D11Output::ReleaseTextures(vlc::RenderContext* context) {
  context->texture_vlc = nullptr;
  context->texture_render_target = nullptr;
  context->texture = nullptr;
}

bool VlcD3D11Output::UpdateOutputCb(void* opaque,
                                    const libvlc_video_render_cfg_t* cfg,
                                    libvlc_video_output_cfg_t* out) {
  const DXGI_FORMAT kRenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

  auto self = reinterpret_cast<VlcD3D11Output*>(opaque);
  auto render_context = &self->render_context_;

  ReleaseTextures(render_context);

  self->SetDimensions(VideoDimensions(cfg->width, cfg->height, 0));

  /* interim texture */
  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.MipLevels = 1;
  texDesc.SampleDesc.Count = 1;
  texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.CPUAccessFlags = 0;
  texDesc.ArraySize = 1;
  texDesc.Format = kRenderFormat;
  texDesc.Height = cfg->height;
  texDesc.Width = cfg->width;
  texDesc.MiscFlags =
      D3D11_RESOURCE_MISC_SHARED /*| D3D11_RESOURCE_MISC_SHARED_NTHANDLE*/;

  if (FAILED(render_context->d3d_device->CreateTexture2D(
          &texDesc, NULL, render_context->texture.put()))) {
    std::cerr << "creating texture failed " << std::endl;
    return false;
  }

  {
    winrt::com_ptr<IDXGIResource> shared_resource;
    HANDLE shared_handle;

    if (FAILED(render_context->texture->QueryInterface(
            __uuidof(IDXGIResource), shared_resource.put_void()))) {
      return false;
    }

    if (FAILED(shared_resource->GetSharedHandle(&shared_handle))) {
      return false;
    }

    if (FAILED(render_context->d3d_device_vlc->OpenSharedResource(
            shared_handle, __uuidof(ID3D11Texture2D),
            render_context->texture_vlc.put_void()))) {
      return false;
    }
  }

  D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
  renderTargetViewDesc.Format = texDesc.Format;
  renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

  if (FAILED(render_context->d3d_device_vlc->CreateRenderTargetView(
          render_context->texture_vlc.get(), &renderTargetViewDesc,
          render_context->texture_render_target.put()))) {
    return false;
  }

  ID3D11RenderTargetView* const targets[1] = {
      render_context->texture_render_target.get()};
  render_context->d3d_context_vlc->OMSetRenderTargets(1, targets, nullptr);

  out->dxgi_format = kRenderFormat;
  out->full_range = true;
  out->colorspace = libvlc_video_colorspace_BT709;
  out->primaries = libvlc_video_primaries_BT709;
  out->transfer = libvlc_video_transfer_func_SRGB;

  self->delegate_->SetTexture(render_context->texture);

  return true;
}

void VlcD3D11Output::SwapCb(void* opaque) {
  auto self = reinterpret_cast<VlcD3D11Output*>(opaque);
  self->delegate_->Present();
}

bool VlcD3D11Output::StartRenderingCb(void* opaque, bool enter) { return true; }

bool VlcD3D11Output::SelectPlaneCb(void* opaque, size_t plane, void* out) {
  ID3D11RenderTargetView** output = static_cast<ID3D11RenderTargetView**>(out);
  auto self = reinterpret_cast<VlcD3D11Output*>(opaque);

  // we only support one packed RGBA plane
  if (plane != 0) {
    return false;
  }

  // we don't really need to return it as we already do the
  // OMSetRenderTargets().
  *output = self->render_context_.texture_render_target.get();
  return true;
}

}  // namespace foxglove
