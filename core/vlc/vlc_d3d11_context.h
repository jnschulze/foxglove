#pragma once

#include "base/error_details.h"
#include "base/status.h"
#include "video/d3d11_output.h"

namespace foxglove {
namespace vlc {

class RenderContext {
 public:
  RenderContext() {}
  ~RenderContext();

  Status<ErrorDetails> Initialize(IDXGIAdapter* preferred_adapter = nullptr);
  Status<ErrorDetails> Update(unsigned int width, unsigned int height, DXGI_FORMAT format);
  void Reset();

  inline ID3D11DeviceContext* d3d_device_context_vlc() const {
    return d3d_device_context_vlc_.get();
  }
  inline ID3D11Texture2D* texture() const { return texture_.get(); }
  inline ID3D11RenderTargetView* render_target() const {
    return texture_render_target_.get();
  }

 private:
  winrt::com_ptr<ID3D11Device> d3d_device_;
  winrt::com_ptr<ID3D11Device> d3d_device_vlc_;
  winrt::com_ptr<ID3D11DeviceContext> d3d_device_context_vlc_;
  winrt::com_ptr<ID3D11Texture2D> texture_;
  winrt::com_ptr<ID3D11Texture2D> texture_vlc_;
  winrt::com_ptr<ID3D11RenderTargetView> texture_render_target_;
  HANDLE texture_shared_handle_ = INVALID_HANDLE_VALUE;
};

}  // namespace vlc
}  // namespace foxglove
