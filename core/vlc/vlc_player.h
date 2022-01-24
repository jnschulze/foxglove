#pragma once

#include "player.h"
#include "vlc_environment.h"

namespace foxglove {

class VlcPlayer : public Player {
 public:
  VlcPlayer(std::shared_ptr<VlcEnvironment> environment);
  virtual ~VlcPlayer() override;

  private:
    std::shared_ptr<VlcEnvironment> environment_;

};

}  // namespace foxglove