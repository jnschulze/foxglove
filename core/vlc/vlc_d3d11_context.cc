#include "vlc/vlc_d3d11_context.h"

#include <iostream>

#ifdef DEBUG_D3D11_LEAKS
#include <dxgidebug.h>
#endif

static void list_dxgi_leaks() {
#ifdef DEBUG_D3D11_LEAKS
  HMODULE dxgidebug_dll = LoadLibrary(TEXT("DXGIDEBUG.DLL"));
  if (dxgidebug_dll) {
    typedef HRESULT(WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void**);
    auto pf_DXGIGetDebugInterface =
        reinterpret_cast<LPDXGIGETDEBUGINTERFACE>(reinterpret_cast<void*>(
            GetProcAddress(dxgidebug_dll, "DXGIGetDebugInterface")));
    if (pf_DXGIGetDebugInterface) {
      winrt::com_ptr<IDXGIDebug> dxgi_debug;
      if (SUCCEEDED(pf_DXGIGetDebugInterface(__uuidof(IDXGIDebug),
                                             dxgi_debug.put_void())))
        dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
    FreeLibrary(dxgidebug_dll);
  }
#endif  // DEBUG_D3D11_LEAKS
}

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#ifdef DEBUG_D3D11_LEAKS
#pragma comment(lib, "dxguid.lib")
#endif

namespace foxglove {
namespace vlc {

RenderContext::~RenderContext() {
  Reset();
#ifdef DEBUG_D3D11_LEAKS
  list_dxgi_leaks();
#endif
}

void RenderContext::Reset() {
  if (texture_shared_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(texture_shared_handle_);
    texture_shared_handle_ = INVALID_HANDLE_VALUE;
  }
}

bool RenderContext::Initialize(IDXGIAdapter* preferred_adapter) {
  UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG_D3D11_LEAKS
  creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  auto hr = D3D11CreateDevice(
      preferred_adapter,
      preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
      nullptr, creation_flags, nullptr, 0, D3D11_SDK_VERSION, d3d_device_.put(),
      nullptr, nullptr);

  if (FAILED(hr)) {
    std::cerr << "D3D11CreateDevice failed: " << hr << std::endl;
    return false;
  }

  {
    winrt::com_ptr<ID3D10Multithread> d3d_multithread;
    hr = d3d_device_->QueryInterface(__uuidof(ID3D10Multithread),
                                     d3d_multithread.put_void());

    if (SUCCEEDED(hr)) {
      d3d_multithread->SetMultithreadProtected(TRUE);
    }
  }

  hr = D3D11CreateDevice(
      preferred_adapter,
      preferred_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      creation_flags |
          D3D11_CREATE_DEVICE_VIDEO_SUPPORT, /* needed for hardware decoding */
      nullptr, 0, D3D11_SDK_VERSION, d3d_device_vlc_.put(), nullptr,
      d3d_device_context_vlc_.put());
  if (FAILED(hr)) {
    std::cerr << "D3D11CreateDevice failed: " << hr << std::endl;
    return false;
  }

  return true;
}

bool RenderContext::Update(unsigned int width, unsigned int height,
                           DXGI_FORMAT format) {
  Reset();

  /* interim texture */
  D3D11_TEXTURE2D_DESC texture_description = {};
  texture_description.MipLevels = 1;
  texture_description.SampleDesc.Count = 1;
  texture_description.BindFlags =
      D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  texture_description.Usage = D3D11_USAGE_DEFAULT;
  texture_description.CPUAccessFlags = 0;
  texture_description.ArraySize = 1;
  texture_description.Format = format;
  texture_description.Width = width;
  texture_description.Height = height;
  texture_description.MiscFlags =
      D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

  texture_ = nullptr;
  auto hr = d3d_device_->CreateTexture2D(&texture_description, nullptr,
                                         texture_.put());
  if (FAILED(hr)) {
    std::cerr << "Creating texture failed: " << hr << std::endl;
    return false;
  }

  {
    winrt::com_ptr<IDXGIResource1> shared_resource;
    if (FAILED(texture_->QueryInterface(__uuidof(IDXGIResource1),
                                        shared_resource.put_void()))) {
      return false;
    }

    if (FAILED(shared_resource->CreateSharedHandle(
            nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            nullptr, &texture_shared_handle_))) {
      std::cerr << "Creating shared handle failed." << std::endl;
      return false;
    }

    winrt::com_ptr<ID3D11Device1> d3d_device_vlc1;
    d3d_device_vlc_->QueryInterface(__uuidof(ID3D11Device1),
                                    d3d_device_vlc1.put_void());

    texture_vlc_ = nullptr;
    if (FAILED(d3d_device_vlc1->OpenSharedResource1(texture_shared_handle_,
                                                    __uuidof(ID3D11Texture2D),
                                                    texture_vlc_.put_void()))) {
      std::cerr << "Opening shared handle failed." << std::endl;
      return false;
    }
  }

  D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc = {};
  render_target_view_desc.Format = format;
  render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

  texture_render_target_ = nullptr;
  if (FAILED(d3d_device_vlc_->CreateRenderTargetView(
          texture_vlc_.get(), &render_target_view_desc,
          texture_render_target_.put()))) {
    return false;
  }

  ID3D11RenderTargetView* const targets[1] = {texture_render_target_.get()};
  d3d_device_context_vlc_->OMSetRenderTargets(1, targets, nullptr);

  return true;
}
}  // namespace vlc

}  // namespace foxglove
