#pragma once

#include <deque>
#include <functional>
#include <mutex>

#include "message_window.h"
#include "thread_checker.h"

namespace foxglove {
namespace windows {

// SingleThreadDispatcher
// All task enqueued will be run on the thread where the dispatcher is created.
class SingleThreadDispatcher {
 public:
  using Task = std::function<void()>;

  SingleThreadDispatcher();
  ~SingleThreadDispatcher();

  void Dispatch(Task task);
  void Terminate();
  bool RunsTasksOnCurrentThread() const {
    return thread_checker_.IsCreationThreadCurrent();
  }

 private:
  ThreadChecker thread_checker_;
  std::mutex tasks_mutex_;
  std::deque<Task> tasks_;
  std::unique_ptr<MessageWindow> message_window_;
  bool terminated_;

  void ProcessTasks();
};

}  // namespace windows
}  // namespace foxglove
