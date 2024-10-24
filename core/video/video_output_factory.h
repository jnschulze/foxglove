#pragma once

#include <memory>

#ifdef _WIN32
#include "video/d3d11_output.h"
#endif

#include "video/pixel_buffer_output.h"

namespace foxglove {

template <typename T>
class VideoOutputFactory {
 public:
  virtual std::unique_ptr<T> CreatePixelBufferOutput(
      std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
      PixelFormat pixel_format) const = 0;

#ifdef _WIN32
  virtual std::unique_ptr<T> CreateD3D11Output(
      std::unique_ptr<D3D11OutputDelegate> output_delegate,
      winrt::com_ptr<IDXGIAdapter> adapter = nullptr) const = 0;
#endif
};

}  // namespace foxglove
