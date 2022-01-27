#pragma once

#include <memory>

#include "video/pixel_buffer_output.h"

namespace foxglove {

class VideoOutputFactory {
 public:
  virtual std::unique_ptr<VideoOutput> CreatePixelBufferOutput(
      std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
      PixelFormat pixel_format) const = 0;
};

}  // namespace foxglove