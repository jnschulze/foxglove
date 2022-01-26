#pragma once

#include <vlcpp/vlc.hpp>

#include "player_environment.h"

namespace foxglove {

class VlcEnvironment : public PlayerEnvironment,
                       public std::enable_shared_from_this<VlcEnvironment> {
 public:
  VlcEnvironment(const std::vector<std::string>& arguments);
  ~VlcEnvironment() override;

  std::unique_ptr<Player> CreatePlayer() override;
  VLC::Instance vlc_instance() const { return vlc_instance_; }

 private:
  VLC::Instance vlc_instance_;
};

}  // namespace foxglove
