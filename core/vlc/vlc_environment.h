#pragma once

#include <vlc/vlc.h>

#include <memory>

#include "base/task_queue.h"
#include "player_environment.h"

namespace foxglove {

struct VlcDeleter {
  void operator()(libvlc_instance_t* instance) { libvlc_release(instance); }
};

class VlcInstance {
 public:
  explicit VlcInstance(libvlc_instance_t* instance) {
    instance_.reset(instance);
  }

  explicit VlcInstance(int argc, const char* const* argv)
      : VlcInstance(libvlc_new(argc, argv)) {}

  libvlc_instance_t* get() const { return instance_.get(); }

  bool isValid() const { return (bool)instance_; }

  operator libvlc_instance_t*() const { return instance_.get(); }

 private:
  std::unique_ptr<libvlc_instance_t, VlcDeleter> instance_;
};

class VlcEnvironment : public PlayerEnvironment,
                       public std::enable_shared_from_this<VlcEnvironment> {
 public:
  VlcEnvironment(std::vector<std::string> arguments,
                 std::shared_ptr<TaskQueue> task_queue);
  ~VlcEnvironment() override;

  std::unique_ptr<Player> CreatePlayer() override;
  VlcInstance* vlc_instance() const { return instance_.get(); }

  TaskQueue* task_runner() const { return task_queue_.get(); }

 private:
  std::vector<std::string> arguments_;
  std::shared_ptr<TaskQueue> task_queue_;
  std::unique_ptr<VlcInstance> instance_;
};

}  // namespace foxglove
