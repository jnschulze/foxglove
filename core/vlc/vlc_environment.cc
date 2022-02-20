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

VlcEnvironment::VlcEnvironment(const std::vector<std::string>& options)
    : VlcEnvironment(
          options, std::make_shared<TaskQueue>(1, "io.jns.foxglove.vlcenv")) {}

VlcEnvironment::VlcEnvironment(const std::vector<std::string>& options,
                               std::shared_ptr<TaskQueue> task_queue)
    : task_queue_(task_queue) {
  if (options.empty()) {
    instance_ = std::make_unique<VlcInstance>(0, nullptr);
  } else {
    auto opts = ToCharArray(options);
    instance_ = std::make_unique<VlcInstance>(
        static_cast<int32_t>(options.size()), opts.get());
  }

#ifndef NDEBUG
  libvlc_set_exit_handler(
      instance_->get(),
      [](void* opaque) { std::cerr << "libvlc exit" << std::endl; }, nullptr);
#endif
}

VlcEnvironment::~VlcEnvironment() {
#ifndef NDEBUG
  std::cerr << "VlcEnvironment::~VlcEnvironment" << std::endl;
#endif
}

std::unique_ptr<Player> VlcEnvironment::CreatePlayer() {
  return std::make_unique<VlcPlayer>(shared_from_this());
}

}  // namespace foxglove