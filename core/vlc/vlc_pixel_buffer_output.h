#pragma once

#include <memory>

#include "video/pixel_buffer_output.h"
#include "vlc/vlc_video_output.h"

namespace foxglove {

class VlcPlayer;
class VlcPixelBufferOutput : public VlcVideoOutput {
 public:
  VlcPixelBufferOutput(std::unique_ptr<PixelBufferOutputDelegate> delegate,
                       PixelFormat format);

  void Attach(libvlc_media_player_t* player) override;

  VideoOutputDelegate* output_delegate() const override {
    return delegate_.get();
  }

 private:
  std::unique_ptr<PixelBufferOutputDelegate> delegate_;
  PixelFormat pixel_format_;

  unsigned Setup(char* chroma, unsigned* width, unsigned* height,
                 unsigned* pitch, unsigned* lines);
  void Cleanup();
  void* OnVideoLock(void** planes);
  void OnVideoUnlock(void* picture, void* const* planes);
  void OnVideoPicture(void* picture);
};

}  // namespace foxglove
