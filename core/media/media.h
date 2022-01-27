#pragma once

#include <filesystem>
#include <future>
#include <map>
#include <string>
#include <string_view>

#include "media/media_source.h"

namespace foxglove {

class Media : public MediaSource {
 public:
  static constexpr auto kMediaTypeFile = "file";
  static constexpr auto kMediaTypeNetwork = "network";
  static constexpr auto kMediaTypeDirectShow = "directShow";

  static std::unique_ptr<Media> create(std::string_view type,
                                       const std::string& url) {
    if (type.compare(kMediaTypeFile) == 0)
      return Media::file(url);
    else if (type.compare(kMediaTypeNetwork) == 0)
      return Media::network(url);
    else
      return Media::directShow(url);
  }

  static std::unique_ptr<Media> file(std::string path) {
    std::unique_ptr<Media> media = std::make_unique<Media>();
    media->resource_ = path;
    media->location_ = "file:///" + path;
    media->media_type_ = kMediaTypeFile;
    return media;
  }

  static std::unique_ptr<Media> network(std::string url) {
    std::unique_ptr<Media> media = std::make_unique<Media>();
    media->resource_ = url;
    media->location_ = url;
    media->media_type_ = kMediaTypeNetwork;
    return media;
  }

  static std::unique_ptr<Media> directShow(std::string resource) {
    std::unique_ptr<Media> media = std::make_unique<Media>();
    media->resource_ = resource;
    media->location_ = resource;
    media->media_type_ = kMediaTypeDirectShow;
    return media;
  }

  const std::string type() const { return "media"; }
  const std::string media_type() const { return media_type_; };
  const std::string resource() const { return resource_; };
  const std::string location() const { return location_; };
  //std::map<std::string, std::string>& metas() const { return metas_; };

 private:
  std::string media_type_;
  std::string resource_;
  std::string location_;
  std::map<std::string, std::string> metas_;
};

}  // namespace foxglove
