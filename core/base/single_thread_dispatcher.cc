#include "single_thread_dispatcher.h"
#include <iostream>
namespace foxglove {
namespace windows {

SingleThreadDispatcher::SingleThreadDispatcher()
    : message_window_(
          std::make_unique<MessageWindow>([this]() { ProcessTasks(); })) {}

SingleThreadDispatcher::~SingleThreadDispatcher() {
  // TODO: make sure all tasks are done
}

/**
 * Call this method from the same thread it was created
*/
void SingleThreadDispatcher::Terminate() {
  if(terminated_.load()) {
    return;
  }

  terminated_.store(true);
 // execute all pending tasks
  ProcessTasks();
}

void SingleThreadDispatcher::Dispatch(Task task) {
  if(terminated_.load()) {
    return;
  }
  auto lock = std::lock_guard(task_guard_);
  std::cout << "Enqueuing task on " << GetCurrentThreadId() << std::endl;

  tasks_.push_back(std::move(task));
  message_window_->WakeUp();
}

void SingleThreadDispatcher::ProcessTasks() {
  std::vector<Task> current_tasks;
  {
    auto lock = std::lock_guard(task_guard_);
    current_tasks = {tasks_.begin(), tasks_.end()};
    tasks_.clear();
  }
  if (current_tasks.empty()) {
    return;
  }
  for (auto& current_task : current_tasks) {
    std::cout << "Running on " << GetCurrentThreadId() << std::endl;
    current_task();
  }
}
}  // namespace windows
}  // namespace foxglove