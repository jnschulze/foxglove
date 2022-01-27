#pragma once

#include "video/video_output.h"

namespace foxglove {

class PixelBufferOutputDelegate : public VideoOutputDelegate {
 public:
  virtual void* LockBuffer(void** buffer,
                           const VideoDimensions& dimensions) = 0;
  virtual void UnlockBuffer(void* user_data){};
  virtual void PresentBuffer(const VideoDimensions& dimensions,
                             void* user_data) = 0;
  virtual ~PixelBufferOutputDelegate() = default;
};

}