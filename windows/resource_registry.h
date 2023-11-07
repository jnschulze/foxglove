#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "player_environment.h"

namespace foxglove {

class EnvironmentRegistry {
 public:
  EnvironmentRegistry();
  std::shared_ptr<PlayerEnvironment> CreateEnvironment(
      std::vector<std::string> args);
  std::shared_ptr<PlayerEnvironment> GetEnvironment(int64_t id);

  void RegisterEnvironment(int64_t id, std::shared_ptr<PlayerEnvironment> env);
  std::shared_ptr<PlayerEnvironment> RemoveEnvironment(int64_t id);
  void Clear();

 private:
  std::mutex map_mutex_;
  std::unordered_map<int64_t, std::shared_ptr<PlayerEnvironment>> items_;
};

class PlayerRegistry {
 public:
  typedef std::function<void(Player* player)> VisitPlayerCallback;

  PlayerRegistry();

  void InsertPlayer(int64_t player_id, std::unique_ptr<Player> player);
  std::unique_ptr<Player> RemovePlayer(int64_t player_id);
  void Visit(VisitPlayerCallback visitor);
  void EraseAll(VisitPlayerCallback visitor);
  void Clear();
  size_t Count();

 private:
  std::mutex map_mutex_;
  std::unordered_map<int64_t, std::unique_ptr<Player>> items_;
};

class PlayerResourceRegistry {
 public:
  PlayerResourceRegistry();

  EnvironmentRegistry* environments() const {
    return environment_registry_.get();
  }

  PlayerRegistry* players() const { return player_registry_.get(); }

 private:
  std::unique_ptr<EnvironmentRegistry> environment_registry_;
  std::unique_ptr<PlayerRegistry> player_registry_;
};

}  // namespace foxglove
