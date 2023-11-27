#include "vlc/vlc_pixel_buffer_output.h"

#include <iostream>

#include "vlc/vlc_player.h"

namespace foxglove {

namespace {
constexpr static char* kVLCFormatBGRA = "BGRA";
constexpr static char* kVLCFormatRGBA = "RGBA";
}  // namespace

VlcPixelBufferOutput::VlcPixelBufferOutput(
    std::unique_ptr<PixelBufferOutputDelegate> delegate, PixelFormat format)
    : delegate_(std::move(delegate)), pixel_format_(format) {}

void VlcPixelBufferOutput::Attach(libvlc_media_player_t* player) {
  libvlc_video_set_callbacks(
      player,
      [](void* opaque, void** planes) -> void* {
        auto instance = reinterpret_cast<VlcPixelBufferOutput*>(opaque);
        return instance->OnVideoLock(planes);
      },
      [](void* opaque, void* picture, void* const* planes) {
        auto instance = reinterpret_cast<VlcPixelBufferOutput*>(opaque);
        instance->OnVideoUnlock(picture, planes);
      },
      [](void* opaque, void* picture) {
        auto instance = reinterpret_cast<VlcPixelBufferOutput*>(opaque);
        return instance->OnVideoPicture(picture);
      },
      this);

  libvlc_video_set_format_callbacks(
      player,
      [](void** opaque, char* chroma, unsigned* width, unsigned* height,
         unsigned* pitches, unsigned* lines) -> unsigned {
        auto instance = reinterpret_cast<VlcPixelBufferOutput*>(*opaque);
        if (instance != nullptr) {
          return instance->Setup(chroma, width, height, pitches, lines);
        }
        return 0;
      },
      [](void* opaque) {
        auto instance = reinterpret_cast<VlcPixelBufferOutput*>(opaque);
        instance->Cleanup();
      });
}

unsigned VlcPixelBufferOutput::Setup(char* chroma, unsigned* width,
                                     unsigned* height, unsigned* pitches,
                                     unsigned* lines) {
  // std::lock_guard<std::mutex> lock(mutex_);

  // if (!IsValid()) {
  //   return 0;
  // }

  {
    const char* vlc_format;
    switch (pixel_format_) {
      case PixelFormat::kFormatBGRA:
        vlc_format = kVLCFormatBGRA;
      default:
        vlc_format = kVLCFormatRGBA;
    }
    auto len = std::char_traits<char>::length(vlc_format);
    assert(len == 4);
    memcpy(chroma, vlc_format, len);
  }

  auto w = *width;
  auto h = *height;

  pitches[0] = w * 4;
  lines[0] = h;

  SetDimensions(VideoDimensions(w, h, pitches[0]));

  return 1;
}

void VlcPixelBufferOutput::Cleanup() {
  std::cerr << "RESET " << std::endl;

  // std::lock_guard<std::mutex> lock(mutex_);

  /*
  {
    std::lock_guard<std::mutex> lk(shutdown_mutex_);
    is_rendering_ = false;
  }

  shutdown_cond_.notify_one();
  */

  // if (!IsValid()) {
  //   return;
  // }

  // SendEmptyFrame();
  // SetDimensions(VideoDimensions());
}

void* VlcPixelBufferOutput::OnVideoLock(void** planes) {
  auto user_data = delegate_->LockBuffer(planes, current_dimensions_);
  assert(planes[0]);
  return user_data;
}

void VlcPixelBufferOutput::OnVideoUnlock(void* user_data, void* const* planes) {
  delegate_->UnlockBuffer(user_data);
}

void VlcPixelBufferOutput::OnVideoPicture(void* user_data) {
  // if (!IsValid()) {
  //   std::cerr << "presentz not valid" << std::endl;
  //   return;
  // }

  delegate_->PresentBuffer(current_dimensions_, user_data);
}

}  // namespace foxglove
