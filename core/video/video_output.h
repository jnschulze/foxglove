#pragma once

#include <functional>

#include "video/video_dimensions.h"

namespace foxglove {

enum class PixelFormat { kNone = 0, kFormatRGBA, kFormatBGRA };

typedef std::function<void(const VideoDimensions& dimensions)>
    VideoDimensionsCallback;

class VideoOutputDelegate {
 public:
  virtual void Shutdown(){};
  virtual ~VideoOutputDelegate() {}
};

class VideoOutput {
 public:
  virtual ~VideoOutput() = default;
  virtual void OnDimensionsChanged(
      VideoDimensionsCallback dimensions_callback) = 0;
  virtual const VideoDimensions& dimensions() const = 0;
  virtual VideoOutputDelegate* output_delegate() const = 0;
};

}  // namespace foxglove