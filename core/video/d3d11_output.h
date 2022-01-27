#pragma once

#include <winrt/base.h>

#include "video/d3d.h"
#include "video/video_output.h"

namespace foxglove {

class D3D11OutputDelegate : public VideoOutputDelegate {
 public:
  virtual void SetTexture(winrt::com_ptr<ID3D11Texture2D> texture) = 0;
  virtual void Present() = 0;
};

}  // namespace foxglove