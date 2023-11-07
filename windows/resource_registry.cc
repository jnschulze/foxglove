#include "resource_registry.h"

#include "player_environment.h"
#include "vlc/vlc_environment.h"

namespace foxglove {

EnvironmentRegistry::EnvironmentRegistry() {}

std::shared_ptr<PlayerEnvironment> EnvironmentRegistry::CreateEnvironment(
    std::vector<std::string> args) {
  auto environment = std::make_shared<VlcEnvironment>(args);
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    items_[environment->id()] = environment;
  }
  return environment;
}

std::shared_ptr<PlayerEnvironment> EnvironmentRegistry::GetEnvironment(
    int64_t id) {
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = items_.find(id);
    if (it != items_.end()) {
      return it->second;
    }
  }
  return {};
}

void EnvironmentRegistry::RegisterEnvironment(
    int64_t id, std::shared_ptr<PlayerEnvironment> env) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  items_[id] = env;
}

std::shared_ptr<PlayerEnvironment> EnvironmentRegistry::RemoveEnvironment(
    int64_t id) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  auto p = items_.extract(id);
  if (!p.empty()) {
    return std::move(p.mapped());
  }
  return {};
}

void EnvironmentRegistry::Clear() {
  std::lock_guard<std::mutex> lock(map_mutex_);
  items_.clear();
}

PlayerRegistry::PlayerRegistry() {}

void PlayerRegistry::InsertPlayer(int64_t player_id,
                                  std::unique_ptr<Player> player) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  items_[player->id()] = std::move(player);
}

std::unique_ptr<Player> PlayerRegistry::RemovePlayer(int64_t player_id) {
  std::unique_ptr<Player> player;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto p = items_.extract(player_id);
    if (!p.empty()) {
      player = std::move(p.mapped());
    }
  }
  return player;
}

void PlayerRegistry::Visit(VisitPlayerCallback visitor) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& kvp : items_) {
    visitor(kvp.second.get());
  }
}

void PlayerRegistry::EraseAll(VisitPlayerCallback visitor) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  auto it = items_.begin();
  while (it != items_.end()) {
    visitor(it->second.get());
    it = items_.erase(it);
  }
}

size_t PlayerRegistry::Count() {
  std::lock_guard<std::mutex> lock(map_mutex_);
  return items_.size();
}

void PlayerRegistry::Clear() {
  std::lock_guard<std::mutex> lock(map_mutex_);
  items_.clear();
}

PlayerResourceRegistry::PlayerResourceRegistry()
    : environment_registry_(std::make_unique<EnvironmentRegistry>()),
      player_registry_(std::make_unique<PlayerRegistry>()) {}

}  // namespace foxglove
