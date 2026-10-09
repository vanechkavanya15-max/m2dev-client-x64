#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "../Core/Result.h"
#include "../World/ActorRegistry.h"

namespace Client::Actor {

class ActorLifecycleManager {
public:
    explicit ActorLifecycleManager(World::ActorRegistry& registry) noexcept;
    
    // Non-copyable, non-movable
    ActorLifecycleManager(const ActorLifecycleManager&) = delete;
    ActorLifecycleManager& operator=(const ActorLifecycleManager&) = delete;
    ActorLifecycleManager(ActorLifecycleManager&&) = delete;
    ActorLifecycleManager& operator=(ActorLifecycleManager&&) = delete;

    [[nodiscard]] Core::Result<void, Core::EntityError> SpawnActor(const World::ActorRecord& record) const noexcept;
    [[nodiscard]] Core::Result<void, Core::EntityError> DespawnActor(World::EntityVid vid) const noexcept;

private:
    World::ActorRegistry& m_registry;
};

} // namespace Client::Actor
