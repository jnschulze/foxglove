#pragma once

#include <memory>

#include "base/thread_safe_map.h"
#include "player_environment.h"

namespace foxglove {

template <typename TEnv, typename TPlayer>
class PlayerRegistryBase {
 public:
  typedef ThreadSafeMap<int64_t, std::unique_ptr<TPlayer>> PlayerMap;
  typedef ThreadSafeMap<int64_t, std::shared_ptr<TEnv>> EnvironmentMap;

  PlayerRegistryBase()
      : environment_map_(std::make_unique<EnvironmentMap>()),
        player_map_(std::make_unique<PlayerMap>()) {}

  EnvironmentMap* environments() const { return environment_map_.get(); }
  PlayerMap* players() const { return player_map_.get(); }

 private:
  std::unique_ptr<EnvironmentMap> environment_map_;
  std::unique_ptr<PlayerMap> player_map_;
};

typedef PlayerRegistryBase<PlayerEnvironment, Player> PlayerRegistry;

}  // namespace foxglove
