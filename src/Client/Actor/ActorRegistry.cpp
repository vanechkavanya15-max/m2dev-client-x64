#include "ActorRegistry.h"
#include <mutex>

namespace Client::Actor {

Core::Result<void, Core::ActorError> ActorRegistry::RegisterActor(ActorRef actor) noexcept
{
    if (!actor) {
        return std::unexpected(Core::ActorError::ActorNotFound);
    }

    const auto id = actor->GetEntityId();

    std::unique_lock lock(m_mutex);
    if (m_actors.contains(id)) {
        return std::unexpected(Core::ActorError::InvalidPosition); // Uzycie dostepnego bledu dla nieprawidlowego stanu, mozna uzyc AlreadyExists jesli dostepne
    }

    m_actors.emplace(id, std::move(actor));
    return {};
}

Core::Result<void, Core::ActorError> ActorRegistry::UnregisterActor(EterBase::EntityId id) noexcept
{
    std::unique_lock lock(m_mutex);
    
    if (auto it = m_actors.find(id); it != m_actors.end()) {
        m_actors.erase(it);
        return {};
    }

    return std::unexpected(Core::ActorError::ActorNotFound);
}

std::optional<ActorRef> ActorRegistry::GetActor(EterBase::EntityId id) const noexcept
{
    std::shared_lock lock(m_mutex);
    if (auto it = m_actors.find(id); it != m_actors.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ActorRegistry::HasActor(EterBase::EntityId id) const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_actors.contains(id);
}

size_t ActorRegistry::GetActorCount() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_actors.size();
}

void ActorRegistry::Clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_actors.clear();
}

} // namespace Client::Actor
