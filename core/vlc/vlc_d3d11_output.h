#pragma once

#include "video/d3d11_output.h"
#include "vlc/vlc_video_output.h"
#include "vlcpp/vlc.hpp"

namespace foxglove {

namespace vlc {

struct RenderContext {
  winrt::com_ptr<ID3D11Device> d3d_device_vlc;
  winrt::com_ptr<ID3D11DeviceContext> d3d_context_vlc;

  winrt::com_ptr<ID3D11Texture2D> texture;
  winrt::com_ptr<ID3D11Texture2D> texture_vlc;
  HANDLE texture_shared_handle;

  winrt::com_ptr<ID3D11Device> d3d_device;
  winrt::com_ptr<ID3D11DeviceContext> d3d_context;

  winrt::com_ptr<ID3D11RenderTargetView> texture_render_target;
};

}  // namespace vlc

class VlcPlayer;
class VlcD3D11Output : public VlcVideoOutput {
 public:
  VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                 IDXGIAdapter* adapter);

  void Attach(VlcPlayer* player) override;
  void Shutdown() override;

  void OnDimensionsChanged(
      VideoDimensionsCallback dimensions_callback) override {
    dimensions_changed_ = dimensions_callback;
  }
  const VideoDimensions& dimensions() const override {
    return current_dimensions_;
  }
  VideoOutputDelegate* output_delegate() const override {
    return delegate_.get();
  }

 private:
  winrt::com_ptr<IDXGIAdapter> adapter_;

  vlc::RenderContext render_context_;

  std::unique_ptr<D3D11OutputDelegate> delegate_;

  void Initialize();
  void SetDimensions(VideoDimensions&);

  static bool SetupCb(void** opaque, const libvlc_video_setup_device_cfg_t* cfg,
                      libvlc_video_setup_device_info_t* out);
  static void CleanupCb(void* opaque);
  static bool UpdateOutputCb(void* opaque, const libvlc_video_render_cfg_t* cfg,
                             libvlc_video_output_cfg_t* out);
  static void SwapCb(void* opaque);
  static bool StartRenderingCb(void* opaque, bool enter);
  static bool SelectPlaneCb(void* opaque, size_t plane, void* out);
  static void ResizeCb(void* opaque,
                       void (*report_size_change)(void* report_opaque,
                                                  unsigned width,
                                                  unsigned height),
                       void* report_opaque);

  VideoDimensions current_dimensions_{};
  VideoDimensionsCallback dimensions_changed_;
};

}  // namespace foxglove