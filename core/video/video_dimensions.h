#pragma once

namespace foxglove {

struct VideoDimensions {
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_row;

  VideoDimensions() : width(0), height(0), bytes_per_row(0) {}

  VideoDimensions(uint32_t width, uint32_t height, uint32_t bytes_per_row)
      : width(width), height(height), bytes_per_row(bytes_per_row) {}

  bool operator==(const VideoDimensions& other) const {
    return width == other.width && height == other.height &&
           bytes_per_row == other.bytes_per_row;
  }
  bool operator!=(const VideoDimensions& other) const {
    return !operator==(other);
  }
};

}  // namespace foxglove
