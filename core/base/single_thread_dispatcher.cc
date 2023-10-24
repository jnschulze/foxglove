#include "single_thread_dispatcher.h"

namespace foxglove {
namespace windows {

SingleThreadDispatcher::SingleThreadDispatcher()
    : message_window_(
          std::make_unique<MessageWindow>([this]() { ProcessTasks(); })) {}

SingleThreadDispatcher::~SingleThreadDispatcher() = default;
/**
 * Call this method from the same thread it was created
 */
void SingleThreadDispatcher::Terminate() {
  {
    // we need to release the lock before calling ProcessTasks
    auto lock = std::lock_guard(task_guard_);

    if (terminated_) {
      return;
    }
    terminated_ = true;
  }
  // execute all pending tasks
  ProcessTasks();
}

void SingleThreadDispatcher::Dispatch(Task task) {
  auto lock = std::lock_guard(task_guard_);

  if (terminated_) {
    return;
  }

  tasks_.push_back(std::move(task));
  message_window_->WakeUp();
}

void SingleThreadDispatcher::ProcessTasks() {
  std::deque<Task> current_tasks;
  {
    auto lock = std::lock_guard(task_guard_);
    tasks_.swap(current_tasks);
  }
  if (current_tasks.empty()) {
    return;
  }
  for (auto& current_task : current_tasks) {
    current_task();
  }
}
}  // namespace windows
}  // namespace foxglove