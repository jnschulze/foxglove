#include "vlc_player.h"

#include <iostream>

namespace foxglove {

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment)
    : environment_(environment) {}

VlcPlayer::~VlcPlayer() { std::cerr << "destroying player" << std::endl; }

void VlcPlayer::Open(std::unique_ptr<Media> media) {
  std::cerr << "VLC PLAYER OPEN " << media->resource() << std::endl;
}

}  // namespace foxglove