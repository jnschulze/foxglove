#pragma once

#include <string>

namespace foxglove {

class MediaSource {
 public:
  virtual ~MediaSource() = default;
  virtual std::string Type() = 0;
};

}  // namespace foxglove