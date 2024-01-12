#pragma once

#include <mutex>

#include "base/error_details.h"
#include "base/status.h"
#include "video/video_output.h"

struct libvlc_media_player_t;

namespace foxglove {
class VlcVideoOutput : public VideoOutput {
 public:
  virtual Status<ErrorDetails> Attach(libvlc_media_player_t* player) = 0;

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
