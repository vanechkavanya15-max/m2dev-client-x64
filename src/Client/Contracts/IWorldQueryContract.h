#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace Client::Contracts
{
    enum class EntityType : uint8_t
    {
        Unknown = 0,
        Player = 1,
        Monster = 2,
        NPC = 3,
        Warp = 4,
        Door = 5,
        ItemGround = 6,
        Stone = 7
    };

    struct EntitySnapshot
    {
        uint32_t vid{0};
        uint32_t vnum{0};
        EntityType type{EntityType::Unknown};
        std::string name;
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float distance{0.0f};
        int currentHpPercent{100};
        bool isHostile{false};
        bool isDead{false};
    };

    using EntityVisitor = void (*)(const EntitySnapshot& entity, void* userData);

    /**
     * @brief Czysty interfejs percepcji swiata gry dla Agentow AI i MCP.
     */
    class IWorldQueryContract
    {
    public:
        virtual ~IWorldQueryContract() = default;

        [[nodiscard]] virtual std::vector<EntitySnapshot> GetEntitiesInRadius(float radius) const = 0;
        [[nodiscard]] virtual std::optional<EntitySnapshot> GetEntityByVID(uint32_t vid) const = 0;
        [[nodiscard]] virtual std::optional<EntitySnapshot> GetNearestHostile() const = 0;
        [[nodiscard]] virtual std::optional<EntitySnapshot> GetNearestItemGround() const = 0;

        virtual int ForEachEntityInRadius(float radius, EntityVisitor visitor, void* userData) const
        {
            if (!visitor)
                return 0;
            auto entities = GetEntitiesInRadius(radius);
            for (const auto& e : entities)
            {
                visitor(e, userData);
            }
            return static_cast<int>(entities.size());
        }
    };
}
