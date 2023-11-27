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

  Media(std::string_view media_type, std::string_view resource,
        std::string_view location)
      : media_type_(media_type), resource_(resource), location_(location) {}

  std::unique_ptr<Media> clone() {
    return std::make_unique<Media>(media_type(), resource(), location());
  }

  Media(const Media& m) : Media(m.media_type(), m.resource(), m.location()) {}

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
    return std::make_unique<Media>(kMediaTypeFile, path, "file:///" + path);
  }

  static std::unique_ptr<Media> network(std::string url) {
    return std::make_unique<Media>(kMediaTypeNetwork, url, url);
  }

  static std::unique_ptr<Media> directShow(std::string resource) {
    return std::make_unique<Media>(kMediaTypeDirectShow, resource, resource);
  }

  const std::string type() const { return "media"; }
  const std::string media_type() const { return media_type_; };
  const std::string resource() const { return resource_; };
  const std::string location() const { return location_; };

 private:
  std::string media_type_;
  std::string resource_;
  std::string location_;
};

}  // namespace foxglove
