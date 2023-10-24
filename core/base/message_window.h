#pragma once

#include <Windows.h>
#include <string>
#include <functional>

namespace foxglove {
namespace windows {

class MessageWindow {
 public:
  using Delegate = std::function<void()>;

  MessageWindow(Delegate taskExecutor);
  ~MessageWindow();

  /**
   * Sends a message to the window so that it can execute the taskExecutor on
   * the thread it was created on.
   */
  void WakeUp();

 private:
  WNDCLASS RegisterWindowClass();
  void ProcessTasks();
  LRESULT HandleMessage(UINT const message, WPARAM const wparam,
                        LPARAM const lparam) noexcept;
  static LRESULT CALLBACK WndProc(HWND const window, UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;
  std::wstring window_class_name_;
  HWND window_handle_;
  Delegate task_executor_;
  DWORD main_thread_id_;
};

}  // namespace windows
}  // namespace foxglove