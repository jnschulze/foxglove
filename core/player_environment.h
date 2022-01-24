#pragma once

#include <memory>

#include "player.h"

namespace foxglove {

class PlayerEnvironment {
 public:
  virtual ~PlayerEnvironment() = default;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }
  virtual std::unique_ptr<Player> CreatePlayer() = 0;
};

}  // namespace foxglove
