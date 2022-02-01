#pragma once

#include "video/video_output.h"

namespace foxglove {

class VlcPlayer;
class VlcVideoOutput : public VideoOutput {
 public:
  virtual void Attach(VlcPlayer* player) = 0;
  virtual void Shutdown() = 0;
};

}  // namespace foxglove