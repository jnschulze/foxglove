#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "base/thread_checker.h"
#include "message_window.h"

namespace foxglove {
namespace windows {

// MainThreadDispatcher
// All task enqueued will be run on the thread where the dispatcher is created.
class MainThreadDispatcher {
 public:
  using Task = std::function<void()>;

  MainThreadDispatcher();
  ~MainThreadDispatcher();

  void Dispatch(Task&& task);
  bool RunsTasksOnCurrentThread() const {
    return thread_checker_.IsCreationThreadCurrent();
  }

 private:
  ThreadChecker thread_checker_;
  std::mutex tasks_mutex_;
  std::vector<Task> tasks_;
  std::unique_ptr<MessageWindow> message_window_;

  void ProcessTasks();
};

}  // namespace windows
}  // namespace foxglove
