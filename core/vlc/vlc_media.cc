#include "vlc/vlc_media.h"

namespace foxglove {

VlcMedia::VlcMedia(std::unique_ptr<Media> media) : media_(std::move(media)) {
  vlc_media_.reset(libvlc_media_new_location(media_->location().c_str()));
}

}  // namespace foxglove
