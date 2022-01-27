#pragma once

#include <memory>

#ifdef _WIN32
#include "video/d3d11_output.h"
#endif

#include "video/pixel_buffer_output.h"

namespace foxglove {

class VideoOutputFactory {
 public:
  virtual std::unique_ptr<VideoOutput> CreatePixelBufferOutput(
      std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
      PixelFormat pixel_format) const = 0;

#ifdef _WIN32
  virtual std::unique_ptr<VideoOutput> CreateD3D11Output(
      std::unique_ptr<D3D11OutputDelegate> output_delegate,
      IDXGIAdapter* adapter = nullptr) const = 0;
#endif
};

}  // namespace foxglove