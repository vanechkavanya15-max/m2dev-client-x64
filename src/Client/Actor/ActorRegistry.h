#pragma once

#include <optional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "../../EterBase/StrongTypes.h"
#include "../../Client/Core/DomainErrors.h"

namespace Client::Actor {

class IActor
{
public:
    virtual ~IActor() = default;
    virtual EterBase::EntityId GetEntityId() const noexcept = 0;
};

using ActorRef = std::shared_ptr<IActor>;

class ActorRegistry
{
public:
    ActorRegistry() = default;
    ~ActorRegistry() = default;

    ActorRegistry(const ActorRegistry&) = delete;
    ActorRegistry& operator=(const ActorRegistry&) = delete;

    Core::Result<void, Core::ActorError> RegisterActor(ActorRef actor) noexcept;
    Core::Result<void, Core::ActorError> UnregisterActor(EterBase::EntityId id) noexcept;
    std::optional<ActorRef> GetActor(EterBase::EntityId id) const noexcept;
    bool HasActor(EterBase::EntityId id) const noexcept;
    size_t GetActorCount() const noexcept;
    void Clear() noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<EterBase::EntityId, ActorRef> m_actors;
};

} // namespace Client::Actor
