#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "closure.h"
#include "logging.h"

namespace foxglove {

class TaskQueue {
 public:
  TaskQueue(size_t num_threads,
            std::optional<std::string> thread_name = std::nullopt);
  ~TaskQueue();
  void Terminate();
  inline bool terminated() const { return terminated_; }

  template <typename F>
  bool Enqueue(F&& task) {
    if (terminated_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(task_pending_mutex_);
    pending_tasks_.emplace_back(std::forward<F>(task));
    lock.unlock();
    task_pending_cv_.notify_one();
    LOG(TRACE) << "Notified condition variable" << std::endl;
    return true;
  }

  bool Enqueue(Closure task) {
    if (terminated_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(task_pending_mutex_);
    pending_tasks_.push_back(task);
    lock.unlock();
    task_pending_cv_.notify_one();
    LOG(TRACE) << "Notified condition variable" << std::endl;
    return true;
  }

 private:
  std::atomic<bool> terminated_;
  std::optional<std::string> thread_name_;
  std::vector<std::thread> workers_;
  std::mutex task_pending_mutex_;
  std::condition_variable task_pending_cv_;
  std::deque<Closure> pending_tasks_;

  void Run();
};
}  // namespace foxglove
