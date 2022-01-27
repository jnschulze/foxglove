#pragma once

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

namespace foxglove {
namespace windows {
class TextureOutlet {
 public:
  virtual int64_t texture_id() const = 0;
};

}  // namespace windows
}  // namespace foxglove
