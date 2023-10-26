#pragma once

#include <windows.h>

namespace foxglove {

class ThreadChecker final {
 public:
  ThreadChecker() : thread_id_(GetCurrentThreadId()) {}
  ~ThreadChecker() = default;

  bool IsCreationThreadCurrent() const {
    return GetCurrentThreadId() == thread_id_;
  }

 private:
  DWORD thread_id_;
};

}  // namespace foxglove
