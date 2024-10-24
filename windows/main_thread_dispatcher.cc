#include "main_thread_dispatcher.h"

#include <cassert>

namespace foxglove {
namespace windows {

MainThreadDispatcher::MainThreadDispatcher()
    : message_window_(
          std::make_unique<MessageWindow>([this]() { ProcessTasks(); })) {
  tasks_.reserve(32);
}

MainThreadDispatcher::~MainThreadDispatcher() {}

void MainThreadDispatcher::Dispatch(Task&& task) {
  if (RunsTasksOnCurrentThread()) {
    task();
  } else {
    {
      const std::lock_guard<std::mutex> lock(tasks_mutex_);
      tasks_.push_back(std::move(task));
    }
    message_window_->WakeUp();
  }
}

void MainThreadDispatcher::ProcessTasks() {
  assert(thread_checker_.IsCreationThreadCurrent());
  std::vector<Task> current_tasks;
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
