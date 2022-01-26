#include "vlc/vlc_environment.h"

#include <iostream>

#include "vlc/vlc_player.h"

namespace foxglove {

namespace {
static std::unique_ptr<const char* []> ToCharArray(
    const std::vector<std::string>& vector) {
  size_t size = vector.size();
  auto array = std::unique_ptr<const char*[]>(new const char*[size]);

  for (auto i = 0; i < size; i++) {
    array[i] = vector[i].c_str();
  }
  return array;
}
}  // namespace

VlcEnvironment::VlcEnvironment(const std::vector<std::string>& options) {
  if (options.empty()) {
    vlc_instance_ = VLC::Instance(0, nullptr);
  } else {
    auto opts = ToCharArray(options);
    vlc_instance_ =
        VLC::Instance(static_cast<int32_t>(options.size()), opts.get());
  }
}

VlcEnvironment::~VlcEnvironment() {
  std::cerr << "destroy vlc env " << std::endl;
}

std::unique_ptr<Player> VlcEnvironment::CreatePlayer() {
  return std::make_unique<VlcPlayer>(shared_from_this());
}

}  // namespace foxglove