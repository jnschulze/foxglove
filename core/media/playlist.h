#pragma once

#include <map>
#include <string>
#include <vector>

#include "media/media.h"
#include "media/media_source.h"

namespace foxglove {
enum PlaylistMode { single, loop, repeat, last_value };

class Playlist : public MediaSource {
 public:
  PlaylistMode& playlist_mode() { return playlist_mode_; }

  /*
    Playlist(std::vector<std::shared_ptr<Media>> medias,
             PlaylistMode playlist_mode = PlaylistMode::single)
        : medias_(medias), playlist_mode_(playlist_mode){};
        */

  const std::string type() const { return "playlist"; }

  virtual const std::vector<std::shared_ptr<Media>>& medias() const = 0;
  virtual void Add(std::shared_ptr<Media> media) = 0;
  virtual void Remove(uint32_t index) = 0;
  virtual void Insert(uint32_t index, std::shared_ptr<Media> media) = 0;
  virtual void Move(uint32_t from, uint32_t to) = 0;
  virtual std::shared_ptr<Media> GetItem(uint32_t index) const = 0;
  virtual size_t length() const = 0;

 protected:
  PlaylistMode playlist_mode_;
};

}  // namespace foxglove
