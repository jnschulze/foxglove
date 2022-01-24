#pragma once

#include <memory>

namespace foxglove {

class Player {
 public:
  virtual ~Player() = default;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }
};

}  // namespace foxglove