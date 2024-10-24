#pragma once

#include <memory>

#include "base/thread_safe_map.h"
#include "player_environment.h"
#include "vlc/vlc_environment.h"
#include "vlc/vlc_player.h"

namespace foxglove {
namespace internal {

template <typename TEnv, typename TPlayer>
class PlayerRegistryBase {
 public:
  typedef ThreadSafeMap<int64_t, std::unique_ptr<TPlayer>> PlayerMap;
  typedef ThreadSafeMap<int64_t, std::shared_ptr<TEnv>> EnvironmentMap;
  typedef TEnv EnvironmentType;
  typedef TPlayer PlayerType;

  PlayerRegistryBase()
      : environment_map_(std::make_unique<EnvironmentMap>()),
        player_map_(std::make_unique<PlayerMap>()) {}

  EnvironmentMap* environments() const { return environment_map_.get(); }
  PlayerMap* players() const { return player_map_.get(); }

 private:
  std::unique_ptr<EnvironmentMap> environment_map_;
  std::unique_ptr<PlayerMap> player_map_;
};

}  // namespace internal

typedef internal::PlayerRegistryBase<VlcEnvironment, VlcPlayer> PlayerRegistry;

}  // namespace foxglove
