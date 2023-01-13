#pragma once

#include <mutex>

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

  winrt::com_ptr<ID3D11Device> d3d_device;
  winrt::com_ptr<ID3D11DeviceContext> d3d_context;

  winrt::com_ptr<ID3D11RenderTargetView> texture_render_target;
};

}  // namespace vlc

class VlcPlayer;
class VlcD3D11Output : public VlcVideoOutput {
 public:
  VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                 winrt::com_ptr<IDXGIAdapter> adapter);
  ~VlcD3D11Output() override;

  void Attach(VlcPlayer* player) override;
  void Shutdown() override;

  VideoOutputDelegate* output_delegate() const override {
    return delegate_.get();
  }

 private:
  winrt::com_ptr<IDXGIAdapter> adapter_;
  std::mutex render_context_mutex_;
  vlc::RenderContext render_context_;

  std::unique_ptr<D3D11OutputDelegate> delegate_;

  bool Initialize();
  static void ReleaseTextures(vlc::RenderContext* context);

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
};

}  // namespace foxglove