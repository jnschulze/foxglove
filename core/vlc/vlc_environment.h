#pragma once

#include <vlcpp/vlc.hpp>

#include "base/task_queue.h"
#include "player_environment.h"

namespace foxglove {

class VlcEnvironment : public PlayerEnvironment,
                       public std::enable_shared_from_this<VlcEnvironment> {
 public:
  VlcEnvironment(const std::vector<std::string>& arguments,
                 std::shared_ptr<TaskQueue> task_queue);
  VlcEnvironment(const std::vector<std::string>& arguments);
  ~VlcEnvironment() override;

  std::unique_ptr<Player> CreatePlayer() override;
  VLC::Instance vlc_instance() const { return vlc_instance_; }

  TaskQueue* task_runner() const { return task_queue_.get(); }

 private:
  std::shared_ptr<TaskQueue> task_queue_;
  VLC::Instance vlc_instance_;
};

}  // namespace foxglove
