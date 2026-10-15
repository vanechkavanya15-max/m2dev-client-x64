#pragma once

#include <cstdint>

/**
 * @file GroundItemModel.h
 * @brief Defines the data model for items dropped on the ground.
 */

/**
 * @brief Represents an instance of an item dropped on the ground in the game world.
 *
 * This structure holds the essential state of a ground item, including its
 * identification, spatial coordinates, and ownership details. It follows C++20
 * guidelines, avoiding Hungarian notation and using standard integer types.
 */
struct GroundItemInstance
{
    /**
     * @brief Unique identifier for this dropped item instance.
     * 
     * Used by the server and client to synchronize this specific item entity.
     */
    uint32_t id{0};

    /**
     * @brief The generic item template ID (VNUM) defining what this item is.
     * 
     * Points to the item's prototype data (e.g., in item_proto).
     */
    uint32_t itemId{0};

    /**
     * @brief The unique ID of the actor who owns or dropped this item.
     * 
     * If 0, the item might not have a specific owner.
     */
    uint32_t ownerId{0};

    /**
     * @brief The X coordinate of the item in the 3D world space.
     */
    float x{0.0f};

    /**
     * @brief The Y coordinate of the item in the 3D world space.
     */
    float y{0.0f};

    /**
     * @brief The Z coordinate of the item in the 3D world space.
     */
    float z{0.0f};
};
