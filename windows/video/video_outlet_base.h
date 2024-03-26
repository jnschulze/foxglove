#pragma once

#include "texture_registry.h"

namespace foxglove {
namespace windows {

class VideoOutletBase {
 public:
  inline TextureRegistration* registration() const {
    return texture_registration_.get();
  }

 protected:
  std::unique_ptr<flutter::TextureVariant> texture_;
  std::unique_ptr<TextureRegistration> texture_registration_;

  inline bool is_valid() const { return texture_registration_->is_valid(); }
};

}  // namespace windows
}  // namespace foxglove
