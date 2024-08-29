#include "vlc/vlc_environment.h"

#include <iostream>

#include "vlc/vlc_player.h"

namespace foxglove {

namespace {

static std::vector<const char*> ToCharArray(
    const std::vector<std::string>& vector) {
  std::vector<const char*> ptrs;
  ptrs.reserve(vector.size());
  for (const auto& item : vector) {
    ptrs.push_back(item.c_str());
  }
  return ptrs;
}

}  // namespace

VlcEnvironment::VlcEnvironment(const std::vector<std::string>& options,
                               std::shared_ptr<TaskQueue> task_queue)
    : task_queue_(task_queue) {
  if (options.empty()) {
    instance_ = std::make_unique<VlcInstance>(0, nullptr);
  } else {
    auto opts = ToCharArray(options);
    instance_ = std::make_unique<VlcInstance>(static_cast<int32_t>(opts.size()),
                                              opts.data());
  }
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
