#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>
#include <compare>
#include <functional>

namespace EterBase {
    // Basic StrongType implementation based on requirements
    template <typename T, typename Tag>
    struct StrongType {
        T value;
        
        explicit constexpr StrongType(T v) : value(v) {}
        constexpr StrongType() : value{} {}
        
        constexpr T Get() const { return value; }
        
        auto operator<=>(const StrongType&) const = default;
    };
    
    struct EntityIdTag {};
    using EntityId = StrongType<uint32_t, EntityIdTag>;
}

namespace std {
    template <typename Tag>
    struct hash<EterBase::StrongType<uint32_t, Tag>> {
        size_t operator()(const EterBase::StrongType<uint32_t, Tag>& id) const {
            return hash<uint32_t>{}(id.Get());
        }
    };
}

using EntityVid = EterBase::EntityId;

namespace Client::World {

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
    bool UpdatePosition(EntityVid vid, float x, float y, float z, float rotation);
    void SetDead(EntityVid vid, bool isDead);
    size_t Count() const;
    void Clear();

private:
    std::unordered_map<EntityVid, ActorRecord> m_actors;
};

} // namespace Client::World
