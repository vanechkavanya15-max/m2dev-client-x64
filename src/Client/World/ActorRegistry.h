#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>
#include <compare>
#include <functional>

#include "../../EterBase/StrongTypes.h"
#include <shared_mutex>

namespace Client::World {

using EntityVid = EterBase::EntityId;

struct ActorRecord {
    EntityVid vid;
    uint32_t race;
    uint8_t type;
    float x;
    float y;
    float z;
    float rotation;
    std::string name;
    uint32_t guildId;
    uint8_t empire;
    bool isDead;
};

class ActorRegistry {
public:
    ActorRegistry() = default;
    ~ActorRegistry() = default;
    
    // Non-copyable, non-movable for a registry
    ActorRegistry(const ActorRegistry&) = delete;
    ActorRegistry& operator=(const ActorRegistry&) = delete;
    ActorRegistry(ActorRegistry&&) = delete;
    ActorRegistry& operator=(ActorRegistry&&) = delete;

    bool RegisterActor(const ActorRecord& record);
    bool UnregisterActor(EntityVid vid);
    std::optional<ActorRecord> GetActor(EntityVid vid) const;
    [[nodiscard]] bool HasActor(EntityVid vid) const;

    template <typename VisitorFn>
    bool VisitActor(EntityVid vid, VisitorFn&& visitor) const {
        std::shared_lock lock(m_mutex);
        auto it = m_actors.find(vid);
        if (it != m_actors.end()) {
            std::forward<VisitorFn>(visitor)(it->second);
            return true;
        }
        return false;
    }

    template <typename VisitorFn>
    bool ModifyActor(EntityVid vid, VisitorFn&& visitor) {
        std::unique_lock lock(m_mutex);
        auto it = m_actors.find(vid);
        if (it != m_actors.end()) {
            std::forward<VisitorFn>(visitor)(it->second);
            return true;
        }
        return false;
    }

    bool UpdatePosition(EntityVid vid, float x, float y, float z, float rotation);
    void SetDead(EntityVid vid, bool isDead);
    [[nodiscard]] bool IsAlive(EntityVid vid) const;
    [[nodiscard]] bool IsDead(EntityVid vid) const;

    void SetMainActorVid(EntityVid vid);
    [[nodiscard]] EntityVid GetMainActorVid() const;

    size_t Count() const;
    void Clear();

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<EntityVid, ActorRecord> m_actors;
    EntityVid m_mainActorVid{0};
};

} // namespace Client::World
