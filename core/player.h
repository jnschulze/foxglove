#pragma once

#include <memory>

#include "media/media.h"

namespace foxglove {

class PlayerEventDelegate {
 public:
  virtual ~PlayerEventDelegate() = default;
};

class Player {
 public:
  virtual ~Player() = default;

  virtual void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) = 0;

  int64_t id() const { return reinterpret_cast<int64_t>(this); }

  virtual void Open(std::unique_ptr<Media> media) = 0;
};

}  // namespace foxglove