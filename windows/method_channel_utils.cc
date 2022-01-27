#include "method_channel_utils.h"

namespace foxglove {
namespace channels {
  /*
template <typename T>
const T* TryGetMapElement(const flutter::EncodableMap* map,
                          const std::string& key) {
  auto it = map->find(key);
  if (it != map->end()) {
    const T* value = std::get_if<T>(&it->second);
    if (value) {
      return value;
    }
  }
  return nullptr;
}

std::optional<int64_t> TryGetIntValue(const flutter::EncodableValue* value) {
  if (value) {
    if (auto val = std::get_if<int32_t>(value)) {
      return *val;
    }

    if (auto val = std::get_if<int64_t>(value)) {
      return *val;
    }
  }
  return std::nullopt;
}

void GetStringList(const flutter::EncodableList* list,
                   std::vector<std::string>& args) {
  if (list) {
    args.reserve(list->size());
    for (const auto& arg : *list) {
      if (auto str = std::get_if<std::string>(&arg)) {
        args.push_back(*str);
      }
    }
  }
}
*/
}  // namespace channels
}  // namespace foxglove
