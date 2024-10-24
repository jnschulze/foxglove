#include "vlc/vlc_environment.h"

#include <iostream>

#include "vlc/vlc_player.h"

namespace foxglove {

namespace {

std::vector<const char*> ToCharArray(const std::vector<std::string>& vector) {
  std::vector<const char*> ptrs;
  ptrs.reserve(vector.size());
  for (const auto& item : vector) {
    ptrs.push_back(item.c_str());
  }
  return ptrs;
}

}  // namespace

VlcEnvironment::VlcEnvironment(std::vector<std::string> arguments,
                               std::shared_ptr<TaskQueue> task_queue)
    : arguments_(
          std::move(arguments)),  // Keep a copy of arguments with the same
                                  // lifetime as this libvlc instance
                                  // as it's not documented whether libvlc will
                                  // internally copy them or not.
      task_queue_(task_queue) {
  if (arguments_.empty()) {
    instance_ = std::make_unique<VlcInstance>(0, nullptr);
  } else {
    auto opts = ToCharArray(arguments_);
    instance_ = std::make_unique<VlcInstance>(static_cast<int32_t>(opts.size()),
                                              opts.data());
  }
}

VlcEnvironment::~VlcEnvironment() {
#ifndef NDEBUG
  std::cerr << "VlcEnvironment::~VlcEnvironment" << std::endl;
#endif
}

std::unique_ptr<VlcPlayer> VlcEnvironment::CreatePlayer() {
  return std::make_unique<VlcPlayer>(shared_from_this());
}

}  // namespace foxglove
