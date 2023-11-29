#pragma once

#include <vlc/vlc.h>

#include <memory>

#include "media/media.h"

namespace foxglove {

struct VlcMediaDeleter {
  void operator()(libvlc_media_t* instance) { libvlc_media_release(instance); }
};

class VlcMedia {
 public:
  VlcMedia(std::unique_ptr<Media> media);
  ~VlcMedia() = default;

  Media* media() const { return media_.get(); }
  libvlc_media_t* vlc_media() const { return vlc_media_.get(); }

 private:
  std::unique_ptr<Media> media_;
  std::unique_ptr<libvlc_media_t, VlcMediaDeleter> vlc_media_;
};

}  // namespace foxglove
