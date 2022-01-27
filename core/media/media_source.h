#pragma once

#include <string>

namespace foxglove {

class MediaSource {
 public:
  virtual ~MediaSource() = default;
  virtual const std::string type() const = 0;
};

}  // namespace foxglove