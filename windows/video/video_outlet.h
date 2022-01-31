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

  int64_t texture_id() const override { return texture_id_; }

  void* LockBuffer(void** buffer, const VideoDimensions& dimensions) override;
  void UnlockBuffer(void* user_data) override;
  void PresentBuffer(const VideoDimensions& dimensions,
                     void* user_data) override;
  void Shutdown() override;
  virtual ~VideoOutlet();

 private:
  mutable std::mutex buffer_mutex_;
  flutter::TextureRegistrar* texture_registrar_ = nullptr;
  std::unique_ptr<FrameBuffer> current_buffer_;
  std::atomic<bool> shutting_down_ = false;
  std::unique_ptr<flutter::TextureVariant> texture_ = nullptr;
  int64_t texture_id_;

  const FlutterDesktopPixelBuffer* CopyPixelBuffer(size_t width, size_t height);
};

}  // namespace windows
}  // namespace foxglove
