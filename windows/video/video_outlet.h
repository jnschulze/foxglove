#pragma once

#include <mutex>

#include "video/pixel_buffer_output.h"
#include "video/video_output.h"
#include "video_outlet_base.h"

namespace foxglove {
namespace windows {
struct FrameBuffer;

class VideoOutlet : public TextureOutlet, public PixelBufferOutputDelegate {
 public:
  VideoOutlet(flutter::TextureRegistrar* texture_registrar);
  virtual ~VideoOutlet();

  void* LockBuffer(void** buffer, const VideoDimensions& dimensions) override;
  void UnlockBuffer(void* user_data) override;
  void PresentBuffer(const VideoDimensions& dimensions,
                     void* user_data) override;
  void Shutdown() override;

 private:
  mutable std::mutex buffer_mutex_;
  std::unique_ptr<FrameBuffer> current_buffer_;
  std::unique_ptr<flutter::TextureVariant> texture_ = nullptr;
  const FlutterDesktopPixelBuffer* CopyPixelBuffer(size_t width, size_t height);
};

}  // namespace windows
}  // namespace foxglove
