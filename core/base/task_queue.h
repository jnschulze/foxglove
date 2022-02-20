#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace foxglove {

class TaskQueue {
 public:
  typedef std::function<void()> Task;
  TaskQueue(size_t num_threads,
            std::optional<std::string> thread_name = std::nullopt);
  ~TaskQueue();

  template <typename F>
  void Enqueue(F&& task) {
    std::unique_lock<std::mutex> lock(task_pending_mutex_);
    pending_tasks_.emplace_back(std::forward<F>(task));
    lock.unlock();
    task_pending_cv_.notify_one();
  }

  void Enqueue(Task task) {
    std::unique_lock<std::mutex> lock(task_pending_mutex_);
    pending_tasks_.push_back(task);
    lock.unlock();
    task_pending_cv_.notify_one();
  }

 private:
  bool done_ = false;
  std::optional<std::string> thread_name_;
  std::vector<std::thread> workers_;
  std::mutex task_pending_mutex_;
  std::condition_variable task_pending_cv_;
  std::deque<Task> pending_tasks_;

  void Run();
};
}  // namespace foxglove
