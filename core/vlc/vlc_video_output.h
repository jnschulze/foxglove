#pragma once

#include <mutex>

#include "video/video_output.h"

namespace foxglove {

class VlcPlayer;
class VlcVideoOutput : public VideoOutput {
 public:
  virtual void Attach(VlcPlayer* player) = 0;

  const VideoDimensions& dimensions() const override {
    return current_dimensions_;
  }

  void OnDimensionsChanged(
      VideoDimensionsCallback dimensions_callback) override {
    const std::lock_guard<std::mutex> lock(callback_mutex_);
    dimensions_changed_ = dimensions_callback;
  }

 protected:
  std::mutex callback_mutex_;
  VideoDimensionsCallback dimensions_changed_;
  VideoDimensions current_dimensions_{};

  void SetDimensions(VideoDimensions&& dimensions) {
    if (current_dimensions_ != dimensions) {
      current_dimensions_ = dimensions;

      {
        const std::lock_guard<std::mutex> lock(callback_mutex_);
        if (dimensions_changed_) {
          dimensions_changed_(current_dimensions_);
        }
      }
    }
  }
};

}  // namespace foxglove