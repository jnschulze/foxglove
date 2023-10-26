#include "main_thread_dispatcher.h"

#include <cassert>

namespace foxglove {
namespace windows {

MainThreadDispatcher::MainThreadDispatcher()
    : terminated_(false),
      message_window_(
          std::make_unique<MessageWindow>([this]() { ProcessTasks(); })) {}

MainThreadDispatcher::~MainThreadDispatcher() { assert(terminated_); }

void MainThreadDispatcher::Terminate() {
  {
    // we need to release the lock before calling ProcessTasks
    const std::lock_guard<std::mutex> lock(tasks_mutex_);
    if (terminated_) {
      return;
    }
    terminated_ = true;
  }

  // execute all pending tasks
  ProcessTasks();
}

void MainThreadDispatcher::Dispatch(Task task) {
  if (RunsTasksOnCurrentThread()) {
    task();
  } else {
    const std::lock_guard<std::mutex> lock(tasks_mutex_);
    if (terminated_) {
      return;
    }
    tasks_.push_back(std::move(task));
    message_window_->WakeUp();
  }
}

void MainThreadDispatcher::ProcessTasks() {
  assert(thread_checker_.IsCreationThreadCurrent());
  std::deque<Task> current_tasks;
  {
    const std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks_.swap(current_tasks);
  }
  if (current_tasks.empty()) {
    return;
  }
  for (const auto& current_task : current_tasks) {
    current_task();
  }
}

}  // namespace windows
}  // namespace foxglove
