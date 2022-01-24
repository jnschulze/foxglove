#include "vlc_player.h"

#include <iostream>

namespace foxglove {

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment)
    : environment_(environment) {}

VlcPlayer::~VlcPlayer() { std::cerr << "destroying player" << std::endl; }

}  // namespace foxglove