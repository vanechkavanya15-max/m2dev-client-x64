#include "ActorLifecycleManager.h"

namespace Client::Actor {

ActorLifecycleManager::ActorLifecycleManager(World::ActorRegistry& registry) noexcept
    : m_registry(registry) {
}

[[nodiscard]] Core::Result<void, Core::EntityError> ActorLifecycleManager::SpawnActor(const World::ActorRecord& record) const noexcept {
    if (m_registry.HasActor(record.vid)) {
        return std::unexpected(Core::EntityError::AlreadyExists);
    }

    if (m_registry.RegisterActor(record)) {
        return {};
    }

    return std::unexpected(Core::EntityError::AlreadyExists);
}

[[nodiscard]] Core::Result<void, Core::EntityError> ActorLifecycleManager::DespawnActor(World::EntityVid vid) const noexcept {
    if (m_registry.UnregisterActor(vid)) {
        return {};
    }
    
    return std::unexpected(Core::EntityError::NotFound);
}

} // namespace Client::Actor
