#include <cassert>
#include <memory>
#include <utility>

namespace foxglove {
template <typename T>
struct rref_impl {
  rref_impl() = delete;
  rref_impl(T&& x) : x{std::move(x)} {}
  rref_impl(rref_impl& other) : x{std::move(other.x)}, is_copied_{true} {
    assert(other.is_copied_ == false);
  }
  rref_impl(rref_impl&& other)
      : x{std::move(other.x)}, is_copied_{std::move(other.is_copied_)} {}
  rref_impl& operator=(rref_impl other) = delete;
  T&& move() { return std::move(x); }

 private:
  T x;
  bool is_copied_ = false;
};

template <typename T>
rref_impl<T> make_rref(T&& x) {
  return rref_impl<T>{std::move(x)};
}
}  // namespace foxglove
