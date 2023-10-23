#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <functional>
#include <atomic>

#include "message_window.h"
namespace foxglove {
namespace windows {
/**
 * SingleThreadDispatcher
 * All task enqueued will be run on the thread where the dispatcherr is created.
 */

class SingleThreadDispatcher {
 public:
  using Task = std::function<void()>;

  SingleThreadDispatcher();
  ~SingleThreadDispatcher();

  void Dispatch(Task task);
  void Terminate();
 private:
  void ProcessTasks();
  std::vector<Task> tasks_;
  std::unique_ptr<MessageWindow> message_window_;
  std::mutex task_guard_;
  std::atomic_bool terminated_;
};

}  // namespace windows
}  // namespace foxglove