#pragma once

#include <flutter/flutter_view.h>

#include <cassert>

namespace foxglove {
namespace windows {

#ifdef HAVE_FLUTTER_TASK_RUNNER
class FlutterTaskRunner {
 public:
  typedef std::function<void()> VoidCallback;

  explicit FlutterTaskRunner(flutter::FlutterView* view)
      : task_runner_(view->GetPlatformTaskRunner()) {
    assert(task_runner_);
  }

  void PostTask(VoidCallback callback) {
    task_runner_->PostTask(std::move(callback));
  }

 private:
  flutter::FlutterTaskRunner* task_runner_;
};
#else
class FlutterTaskRunner {
 public:
  typedef std::function<void()> VoidCallback;

  explicit FlutterTaskRunner(flutter::FlutterView* view) {}

  void PostTask(VoidCallback callback) { callback(); }
};
#endif

}  // namespace windows
}  // namespace foxglove