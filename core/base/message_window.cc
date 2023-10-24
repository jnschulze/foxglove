#include "message_window.h"

#include <cassert>
#include <iostream>

namespace foxglove {
namespace windows {

void PrintLastError() {
  auto error = GetLastError();
  LPWSTR message = nullptr;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 reinterpret_cast<LPWSTR>(&message), 0, NULL);
  OutputDebugString(message);
  LocalFree(message);
}

MessageWindow::MessageWindow(Delegate taskExecutor)
    : task_executor_(taskExecutor), main_thread_id_(GetCurrentThreadId()) {
  WNDCLASS window_class = RegisterWindowClass();
  window_handle_ =
      CreateWindowEx(0, window_class.lpszClassName, L"", 0, 0, 0, 0, 0,
                     HWND_MESSAGE, nullptr, window_class.hInstance, nullptr);

  if (window_handle_) {
    SetLastError(0);
    if (SetWindowLongPtr(window_handle_, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(this)) == 0) {
      PrintLastError();
    }
  } else {
    PrintLastError();
  }
}

MessageWindow::~MessageWindow() {
  if (window_handle_) {
    DestroyWindow(window_handle_);
    window_handle_ = nullptr;
  }
  UnregisterClass(window_class_name_.c_str(), nullptr);
}

void MessageWindow::WakeUp() {
  if (!PostMessage(window_handle_, WM_NULL, 0, 0)) {
    std::cerr << "Failed to wakeup window" << std::endl;
  }
}

WNDCLASS MessageWindow::RegisterWindowClass() {
  window_class_name_ = L"FlutterPluginMessageWindow";

  WNDCLASS window_class{};
  window_class.hCursor = nullptr;
  window_class.lpszClassName = window_class_name_.c_str();
  window_class.style = 0;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.hIcon = nullptr;
  window_class.hbrBackground = 0;
  window_class.lpszMenuName = nullptr;
  window_class.lpfnWndProc = WndProc;
  RegisterClass(&window_class);
  return window_class;
}

void MessageWindow::ProcessTasks() {
  assert(main_thread_id_ == GetCurrentThreadId());
  task_executor_();
}

LRESULT MessageWindow::HandleMessage(UINT const message, WPARAM const wparam,
                                     LPARAM const lparam) noexcept {
  switch (message) {
    case WM_TIMER:
    case WM_NULL:
      ProcessTasks();
      return 0;
  }
  return DefWindowProc(window_handle_, message, wparam, lparam);
}

LRESULT MessageWindow::WndProc(HWND const window, UINT const message,
                               WPARAM const wparam,
                               LPARAM const lparam) noexcept {
  if (auto* that = reinterpret_cast<MessageWindow*>(
          GetWindowLongPtr(window, GWLP_USERDATA))) {
    return that->HandleMessage(message, wparam, lparam);
  } else {
    return DefWindowProc(window, message, wparam, lparam);
  }
}

}  // namespace windows
}  // namespace foxglove