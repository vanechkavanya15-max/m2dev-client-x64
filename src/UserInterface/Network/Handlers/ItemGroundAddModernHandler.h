#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

/**
 * @brief Represents the data structure for dropping an item on the ground.
 * 
 * This structure strictly defines the packet layout expected from the network stream
 * when an item is added to the ground. It uses exact alignment matching the C++20/23 standard
 * and network protocols.
 */
#pragma pack(push, 1)
struct ItemGroundAddModernPacket
{
    /** @brief The protocol header identifying this specific packet. */
    uint8_t header;

    /** @brief The X coordinate in the global world. */
    int32_t x;

    /** @brief The Y coordinate in the global world. */
    int32_t y;

    /** @brief The Z coordinate in the global world. */
    int32_t z;

    /** @brief The unique virtual identifier of the item instance. */
    uint32_t id;

    /** @brief The virtual number of the item. */
    uint32_t vnum;
};
#pragma pack(pop)
static_assert(sizeof(ItemGroundAddModernPacket) == 21, "ItemGroundAddModernPacket must be exactly 21 bytes");

/**
 * @brief Event triggered when an item is added to the ground.
 * 
 * This event is published to the EventBus to notify other subsystems (e.g., Python UI)
 * that a new item drop has occurred, avoiding direct coupling.
 */
struct ItemGroundAddEvent : public UserInterface::Core::IEvent
{
    /** @brief The unique entity identifier for the dropped item instance. */
    EterBase::EntityId dropVid;

    /** @brief The virtual number defining the type of the item. */
    EterBase::ItemVnum itemVnum;

    /** @brief The X coordinate of the item drop. */
    int32_t x;

    /** @brief The Y coordinate of the item drop. */
    int32_t y;

    /** @brief The Z coordinate of the item drop. */
    int32_t z;

    /**
     * @brief Constructs a new ItemGroundAddEvent.
     * @param dropVid The unique entity ID of the item drop.
     * @param itemVnum The virtual number of the item.
     * @param x The X coordinate.
     * @param y The Y coordinate.
     * @param z The Z coordinate.
     */
    ItemGroundAddEvent(EterBase::EntityId dropVid, EterBase::ItemVnum itemVnum, int32_t x, int32_t y, int32_t z)
        : dropVid(dropVid), itemVnum(itemVnum), x(x), y(y), z(z) {}
};

/**
 * @brief Handles the modern 'Item Ground Add' network packet (C++23).
 * 
 * This handler processes the network packet that signifies an item has been dropped
 * onto the ground in the game world. It strictly validates the incoming buffer
 * and delegates business logic via EventBus.
 */
class ItemGroundAddModernHandler
{
public:
    /**
     * @brief Processes the item ground add packet buffer.
     * 
     * @param buffer The binary span representing the network packet payload.
     * @return EterBase::PacketResult<void> representing success or domain-specific error.
     */
    static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer);
};
