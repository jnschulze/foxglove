#pragma once

#include <vlc/vlc.h>

#include <mutex>

#include "video/d3d11_output.h"
#include "vlc/vlc_d3d11_context.h"
#include "vlc/vlc_video_output.h"

namespace foxglove {

class VlcPlayer;
class VlcD3D11Output : public VlcVideoOutput {
 public:
  VlcD3D11Output(std::unique_ptr<D3D11OutputDelegate> delegate,
                 winrt::com_ptr<IDXGIAdapter> adapter);
  ~VlcD3D11Output() override;

  void Attach(libvlc_media_player_t* player) override;

  VideoOutputDelegate* output_delegate() const override {
    return delegate_.get();
  }

 private:
  std::unique_ptr<D3D11OutputDelegate> delegate_;
  winrt::com_ptr<IDXGIAdapter> adapter_;
  std::mutex render_context_mutex_;
  vlc::RenderContext render_context_;

  bool Initialize();

  static bool SetupCb(void** opaque, const libvlc_video_setup_device_cfg_t* cfg,
                      libvlc_video_setup_device_info_t* out);
  static void CleanupCb(void* opaque);
  static bool UpdateOutputCb(void* opaque, const libvlc_video_render_cfg_t* cfg,
                             libvlc_video_output_cfg_t* out);
  static void SwapCb(void* opaque);
  static bool StartRenderingCb(void* opaque, bool enter);
  static bool SelectPlaneCb(void* opaque, size_t plane, void* out);
  static void ResizeCb(
      void* opaque, libvlc_video_output_resize_cb report_size_change,
      libvlc_video_output_mouse_move_cb report_mouse_move,
      libvlc_video_output_mouse_press_cb report_mouse_pressed,
      libvlc_video_output_mouse_release_cb report_mouse_released,
      void* report_opaque);
};

}  // namespace foxglove
