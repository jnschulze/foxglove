#include "video_outlet.h"

#include <csignal>
#include <iostream>

namespace foxglove {
namespace windows {

namespace {
static size_t AlignBufferSize(size_t width) { return (width + 31) & -32; }
}  // namespace

struct FrameBuffer {
 public:
  typedef void (*VoidCallback)(void*);
  explicit FrameBuffer(size_t size, VoidCallback release_callback,
                       void* release_context = nullptr) {
    buffer_size_ = AlignBufferSize(size);
    data_.reset(new uint8_t[buffer_size_]);

    flutter_pixel_buffer_.buffer = data_.get();
    flutter_pixel_buffer_.release_callback = release_callback;
    flutter_pixel_buffer_.release_context = release_context;
  }
  ~FrameBuffer() { std::cerr << "DESTROY FB " << std::endl; }

  inline size_t buffer_size() const { return buffer_size_; }
  inline uint8_t* data() const { return data_.get(); }
  inline const FlutterDesktopPixelBuffer* flutter_buffer() const {
    return &flutter_pixel_buffer_;
  }
  inline void set_width(size_t width) { flutter_pixel_buffer_.width = width; }
  inline void set_height(size_t height) {
    flutter_pixel_buffer_.height = height;
  }

 private:
  FlutterDesktopPixelBuffer flutter_pixel_buffer_{};
  size_t buffer_size_;
  std::unique_ptr<uint8_t[]> data_;
};

VideoOutlet::VideoOutlet(flutter::TextureRegistrar* texture_registrar)
    : TextureOutlet(texture_registrar) {
  texture_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [=](size_t width, size_t height) -> const FlutterDesktopPixelBuffer* {
            return CopyPixelBuffer(width, height);
          }));
  texture_id_ = texture_registrar_->RegisterTexture(texture_.get());
}

const FlutterDesktopPixelBuffer* VideoOutlet::CopyPixelBuffer(size_t width,
                                                              size_t height) {
  // Gets unlocked in FlutterDesktopPixelBuffer's |release_callback|
  // once Flutter has uploaded the buffer to the GPU.
  buffer_mutex_.lock();

  if (valid() && current_buffer_) {
    return current_buffer_->flutter_buffer();
  }

  buffer_mutex_.unlock();
  return nullptr;
}

void* VideoOutlet::LockBuffer(void** buffer,
                              const VideoDimensions& dimensions) {
  assert(dimensions.bytes_per_row >= dimensions.width * 4);
  size_t required_size = dimensions.bytes_per_row * dimensions.height;
  assert(required_size != 0);

  // Gets unlocked in |UnlockBuffer|
  buffer_mutex_.lock();

  // Only realloc the buffer if it's too small.
  if (!current_buffer_ || current_buffer_->buffer_size() < required_size) {
    current_buffer_ = std::make_unique<FrameBuffer>(
        required_size,
        [](void* data) {
          // Unlocks the mutex acquired in |CopyPixelBuffer|.
          // auto mutex = reinterpret_cast<std::mutex*>(data);
          // mutex->unlock();

          auto instance = reinterpret_cast<VideoOutlet*>(data);
          instance->buffer_mutex_.unlock();
        },
        /*&buffer_mutex_*/ this);
  }

  // Set the real width/height
  current_buffer_->set_width(dimensions.width);
  current_buffer_->set_height(dimensions.height);

  *buffer = current_buffer_->data();
  return nullptr;
}

void VideoOutlet::UnlockBuffer(void* user_data) { buffer_mutex_.unlock(); }

void VideoOutlet::PresentBuffer(const VideoDimensions& dimensions,
                                void* user_data) {
  if (valid()) {
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
  }
}

void VideoOutlet::Shutdown() { Unregister(); }

VideoOutlet::~VideoOutlet() { Unregister(); }

}  // namespace windows

}  // namespace foxglove
