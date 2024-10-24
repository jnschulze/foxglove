#pragma once

#include <memory>

#include "player.h"

namespace foxglove {

template <typename TPlayer>
class PlayerEnvironment {
 public:
  virtual ~PlayerEnvironment() = default;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }
  virtual std::unique_ptr<TPlayer> CreatePlayer() = 0;
};

}  // namespace foxglove
