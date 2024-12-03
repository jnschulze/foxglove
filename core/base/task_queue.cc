#include "task_queue.h"

#include <cassert>

#include "logging.h"

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

    LOG(LOG_TRACE) << "Waiting on task condition variable" << std::endl;
    task_pending_cv_.wait(
        lock, [this]() { return terminated_ || !pending_tasks_.empty(); });
    LOG(LOG_TRACE) << "Worker woke up" << std::endl;

    if (terminated_) {
      LOG(LOG_TRACE) << "Worker terminated" << std::endl;
      return;
    }

    std::deque<Closure> tasks;
    pending_tasks_.swap(tasks);
    lock.unlock();

    LOG(LOG_TRACE) << "Attempting to run tasks" << std::endl;
    for (const auto& task : tasks) {
      task();
    }
    LOG(LOG_TRACE) << "Ran tasks" << std::endl;
  }
}
}  // namespace foxglove
