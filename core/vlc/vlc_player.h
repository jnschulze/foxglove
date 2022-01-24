#pragma once

#include "player.h"
#include "vlc_environment.h"

namespace foxglove {

class VlcPlayer : public Player {
 public:
  VlcPlayer(std::shared_ptr<VlcEnvironment> environment);
  ~VlcPlayer() override;

  void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) override {
    event_delegate_ = std::move(event_delegate);
  }

  void Open(std::unique_ptr<Media> media) override;

 private:
  std::shared_ptr<VlcEnvironment> environment_;
  std::unique_ptr<PlayerEventDelegate> event_delegate_;
};

}  // namespace foxglove