#pragma once

#include <cstdint>
#include <string_view>
#include <span>

namespace Core::Events
{
#pragma pack(push, 1)

    /**
     * @brief Represents a change in a character's HP.
     * 
     * This event is triggered when an entity (player, monster, etc.) takes damage
     * or is healed, resulting in a new HP value.
     */
    struct HpChange
    {
        uint32_t targetId; ///< The unique identifier of the entity whose HP changed.
        uint32_t currentHp; ///< The entity's new current HP.
        uint32_t maxHp; ///< The entity's maximum HP.
    };

    /**
     * @brief Represents a new entity spawning in the game world.
     * 
     * This event is triggered when a new mob, NPC, or player appears in the
     * client's view range.
     */
    struct EntitySpawn
    {
        uint32_t instance; ///< The unique identifier of the newly spawned entity.
        uint32_t type; ///< The type/vnum of the spawned entity (e.g., mob ID).
        int32_t x; ///< The X coordinate of the spawn location.
        int32_t y; ///< The Y coordinate of the spawn location.
        int32_t z; ///< The Z coordinate of the spawn location.
    };

    /**
     * @brief Represents an item dropping on the ground.
     * 
     * This event is triggered when an item is dropped by a mob, player, or
     * other source into the game world.
     */
    struct ItemDrop
    {
        uint32_t id; ///< The unique identifier of the dropped item instance.
        uint32_t vnum; ///< The item type/vnum.
        uint16_t count; ///< The quantity of the dropped item.
        int32_t x; ///< The X coordinate where the item was dropped.
        int32_t y; ///< The Y coordinate where the item was dropped.
        int32_t z; ///< The Z coordinate where the item was dropped.
    };

#pragma pack(pop)
} // namespace Core::Events
