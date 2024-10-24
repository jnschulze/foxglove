#pragma once

namespace foxglove {

namespace internal {

// Based on
// https://github.com/flutter/engine/blob/70a1106b509ea3f34febca59967ed7a76c05ce33/fml/make_copyable.h

template <typename T>
class CopyableLambda {
 public:
  explicit CopyableLambda(T func)
      : impl_(std::make_shared<Impl>(std::move(func))) {}
  template <typename... ArgType>
  auto operator()(ArgType&&... args) const {
    return impl_->func_(std::forward<ArgType>(args)...);
  }

 private:
  class Impl {
   public:
    explicit Impl(T func) : func_(std::move(func)) {}
    T func_;
  };
  std::shared_ptr<Impl> impl_;
};

}  // namespace internal

// Provides a wrapper for a move-only lambda that is implictly convertable to an
// std::function.
//
// std::function is copyable, but if a lambda captures an argument with a
// move-only type, the lambda itself is not copyable. In order to use the lambda
// in places that accept std::functions, we provide a copyable object that wraps
// the lambda and is implicitly convertable to an std::function.
//
// EXAMPLE:
//
// std::unique_ptr<Foo> foo = ...
// std::function<int()> func =
//     MakeCopyable([bar = std::move(foo)]() { return bar->count(); });
//
// Notice that the return type of MakeCopyable is rarely used directly. Instead,
// callers typically erase the type by implicitly converting the return value
// to an std::function.
template <typename T>
internal::CopyableLambda<T> MakeCopyable(T lambda) {
  return internal::CopyableLambda<T>(std::move(lambda));
}

}  // namespace foxglove
