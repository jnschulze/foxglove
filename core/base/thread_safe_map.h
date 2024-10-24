#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace foxglove {

template <typename TKey, typename TValue>
class ThreadSafeMapBase {
 public:
  void Set(const TKey& key, TValue&& value) {
    std::unique_lock lock(this->map_mutex_);
    this->items_[key] = std::forward<TValue>(value);
  }

  void Clear() {
    std::unique_lock lock(this->map_mutex_);
    this->items_.clear();
  }

  size_t Count() const {
    std::shared_lock lock(this->map_mutex_);
    return this->items_.size();
  }

  TValue Remove(const TKey& key) {
    std::unique_lock lock(this->map_mutex_);
    auto p = items_.extract(key);
    if (!p.empty()) {
      return std::move(p.mapped());
    }
    return {};
  }

  void RemoveAll(std::function<void(const TKey&, TValue&)> visitor) {
    std::unique_lock lock(this->map_mutex_);
    auto it = items_.begin();
    while (it != items_.end()) {
      visitor(it->first, it->second);
      it = items_.erase(it);
    }
  }

 protected:
  std::shared_mutex map_mutex_;
  std::unordered_map<TKey, TValue> items_;
};

template <typename TKey, typename TValue>
class ThreadSafeMap : public ThreadSafeMapBase<TKey, TValue> {
 public:
  TValue* Get(const TKey& key) {
    std::shared_lock lock(this->map_mutex_);
    const auto it = this->items_.find(key);
    if (it == this->items_.end()) {
      return nullptr;
    }
    return &it->second;
  }
};

// Specialization for unique_ptr<T> values
template <typename TKey, typename TValue>
class ThreadSafeMap<TKey, std::unique_ptr<TValue>>
    : public ThreadSafeMapBase<TKey, std::unique_ptr<TValue>> {
 public:
  TValue* Get(const TKey& key) {
    std::shared_lock lock(this->map_mutex_);
    const auto it = this->items_.find(key);
    if (it == this->items_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  void RemoveAll(std::function<void(const TKey&, TValue*)> visitor) {
    std::unique_lock lock(this->map_mutex_);
    auto it = items_.begin();
    while (it != items_.end()) {
      visitor(it->first, it->second.get());
      it = items_.erase(it);
    }
  }
};

// Specialization for shared_ptr<T> values
template <typename TKey, typename TValue>
class ThreadSafeMap<TKey, std::shared_ptr<TValue>>
    : public ThreadSafeMapBase<TKey, std::shared_ptr<TValue>> {
 public:
  std::shared_ptr<TValue> Get(const TKey& key) {
    std::shared_lock lock(this->map_mutex_);
    const auto it = this->items_.find(key);
    if (it == this->items_.end()) {
      return nullptr;
    }
    return it->second;
  }
};

}  // namespace foxglove
