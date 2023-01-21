#include "task_queue.h"

#include <cassert>

#if 0
#include <atlconv.h>
#include <processthreadsapi.h>
#endif

namespace foxglove {
TaskQueue::TaskQueue(size_t num_threads, std::optional<std::string> thread_name)
    : thread_name_(thread_name) {
  assert(num_threads > 0);
  for (auto i = 0; i < num_threads; i++) {
    workers_.emplace_back(&TaskQueue::Run, this);
  }
}

TaskQueue::~TaskQueue() {
  Terminate();
  for (auto& t : workers_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void TaskQueue::Terminate() {
  terminated_ = true;
  task_pending_cv_.notify_all();
}

void TaskQueue::Run() {
#if 0
  if (thread_name_.has_value()) {
    auto name = thread_name_.value();
    auto wname = std::wstring(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wname.c_str());
  }
#endif

  while (true) {
    std::unique_lock<std::mutex> lock(task_pending_mutex_);

    task_pending_cv_.wait(
        lock, [this]() { return terminated_ || !pending_tasks_.empty(); });

    if (terminated_) {
      return;
    }

    if (!pending_tasks_.empty()) {
      auto task = std::move(pending_tasks_.front());
      pending_tasks_.pop_front();
      lock.unlock();
      task();
    }
  }
}
}  // namespace foxglove
