#pragma once

#include <mutex>
#include <optional>
#include <vlcpp/vlc.hpp>

#include "media/playlist.h"

namespace foxglove {
class VlcPlaylist : public Playlist {
 public:
  typedef std::function<void()> UpdateCallback;

  VlcPlaylist();
  ~VlcPlaylist() override;

  // |Playlist|
  void Add(std::shared_ptr<Media> media) override;

  // |Playlist|
  void Remove(uint32_t index) override;

  // |Playlist|
  void Insert(uint32_t index, std::shared_ptr<Media> media) override;

  // |Playlist|
  void Move(uint32_t from, uint32_t to) override;

  // |Playlist|
  const std::vector<std::shared_ptr<Media>>& medias() const override {
    return media_list_;
  }

  // |Playlist|
  size_t length() const override {
    const std::lock_guard<std::mutex> lock(mutex_);
    return medias().size();
  }

  // |Playlist|
  std::shared_ptr<Media> GetItem(uint32_t index) const override {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (index < media_list_.size()) {
      return media_list_[index];
    }
    return {};
  }

  void OnUpdate(UpdateCallback update_callback) {
    const std::lock_guard<std::mutex> lock(update_callback_mutex_);
    update_callback_ = std::move(update_callback);
  }

  VLC::MediaList& vlc_media_list() { return vlc_media_list_; };

  std::optional<int> GetIndexOfItem(VLC::Media& media) {
    int index;
    vlc_media_list_.lock();
    index = vlc_media_list_.indexOfItem(media);
    vlc_media_list_.unlock();
    return index >= 0 ? std::make_optional(index) : std::nullopt;
  }

 private:
  mutable std::mutex mutex_;
  std::mutex update_callback_mutex_;
  VLC::MediaList vlc_media_list_;
  std::vector<std::shared_ptr<Media>> media_list_;
  UpdateCallback update_callback_;

  void NotifyUpdate() {
    const std::lock_guard<std::mutex> lock(update_callback_mutex_);
    if (update_callback_) {
      update_callback_();
    }
  }
};
}  // namespace foxglove
