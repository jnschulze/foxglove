#pragma once

#include <Windows.h>

#include <functional>
#include <string>

#include "thread_checker.h"

namespace foxglove {
namespace windows {

class MessageWindow {
 public:
  using Delegate = std::function<void()>;

  MessageWindow(Delegate task_executor);
  ~MessageWindow();

  // Sends a message to the window so that it can execute the task_executor on
  // the thread it was created on.
  void WakeUp();

 private:
#ifndef NDEBUG
  ThreadChecker thread_checker_;
#endif
  std::wstring window_class_name_;
  HWND window_handle_;
  Delegate task_executor_;

  WNDCLASS RegisterWindowClass();
  void ProcessTasks();
  LRESULT HandleMessage(UINT const message, WPARAM const wparam,
                        LPARAM const lparam) noexcept;
  static LRESULT CALLBACK WndProc(HWND const window, UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;
};

}  // namespace windows
}  // namespace foxglove
