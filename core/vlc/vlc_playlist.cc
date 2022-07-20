#include "vlc/vlc_playlist.h"

#include <iostream>

namespace foxglove {

namespace {

template <typename t>
void MoveItem(std::vector<t>& v, size_t oldIndex, size_t newIndex) {
  if (oldIndex > newIndex)
    std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex,
                v.rend() - newIndex);
  else
    std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1,
                v.begin() + newIndex + 1);
}

}  // namespace

VlcPlaylist::VlcPlaylist() : vlc_media_list_(VLC::MediaList()) {}

VlcPlaylist::~VlcPlaylist() {}

void VlcPlaylist::Add(std::shared_ptr<Media> media) {
  VLC::Media vlc_media =
      VLC::Media(media->location(), VLC::Media::FromLocation);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    vlc_media_list_.lock();
    vlc_media_list_.addMedia(vlc_media);
    vlc_media_list_.unlock();
    media_list_.emplace_back(media);
  }
  NotifyUpdate();
}

void VlcPlaylist::Remove(uint32_t index) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= media_list_.size()) {
      return;
    }
    vlc_media_list_.lock();
    vlc_media_list_.removeIndex(index);
    media_list_.erase(media_list_.begin() + index);
    vlc_media_list_.unlock();
  }
  NotifyUpdate();
}

void VlcPlaylist::Insert(uint32_t index, std::shared_ptr<Media> media) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index > media_list_.size()) {
      return;
    }
    VLC::Media vlc_media =
        VLC::Media(media->location(), VLC::Media::FromLocation);
    vlc_media_list_.lock();
    vlc_media_list_.insertMedia(vlc_media, index);
    media_list_.insert(media_list_.begin() + index, media);
    vlc_media_list_.unlock();
  }
  NotifyUpdate();
}

void VlcPlaylist::Move(uint32_t from, uint32_t to) {
  if (from == to) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (from >= media_list_.size() || to >= media_list_.size()) {
      return;
    }
    vlc_media_list_.lock();
    MoveItem(media_list_, from, to);
    VLC::Media vlc_media =
        VLC::Media(vlc_media_list_.itemAtIndex(from).get()->mrl(),
                   VLC::Media::FromLocation);
    vlc_media_list_.removeIndex(from);
    vlc_media_list_.insertMedia(vlc_media, to);
    vlc_media_list_.unlock();
  }
  NotifyUpdate();
}

}  // namespace foxglove
