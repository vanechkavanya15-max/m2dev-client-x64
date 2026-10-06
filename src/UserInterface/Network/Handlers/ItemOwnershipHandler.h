#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
    /** @brief The maximum length of a character name. */
    constexpr size_t CHARACTER_NAME_MAX_LEN = 64;

#pragma pack(push, 1)
    /**
     * @brief Structure representing the Item Ownership network packet.
     * Matches the byte-layout of TPacketGCItemOwnership exactly (using uint16_t header and length).
     */
    struct PacketItemOwnership
    {
        /** @brief The protocol header identifying this specific packet. */
        uint16_t header;

        /** @brief The total length of the packet. */
        uint16_t length;

        /** @brief The unique virtual identifier of the item instance. */
        uint32_t id;

        /** @brief The name of the character who owns the item. */
        char name[CHARACTER_NAME_MAX_LEN + 1];
    };
#pragma pack(pop)

    /**
     * @brief Event emitted when an item's ownership is assigned or updated.
     * Subsystems can subscribe to this event to safely update without direct coupling.
     */
    struct ItemOwnershipEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        std::string name;

        /**
         * @brief Constructs the item ownership event.
         * @param id The entity ID of the item.
         * @param name The name of the new owner.
         */
        ItemOwnershipEvent(EterBase::EntityId id, std::string name)
            : id(id), name(std::move(name)) {}
    };

    /**
     * @brief Processes the item ownership network packet buffer using C++23 patterns.
     * @param buffer The binary span representing the network packet payload.
     * @return EterBase::PacketResult<void> representing success or error.
     */
    EterBase::PacketResult<void> ProcessItemOwnership(std::span<const uint8_t> buffer);

    /**
     * @brief Legacy compatibility handler for the item ownership packet.
     * @param buffer The binary span representing the network packet payload.
     * @return true if the buffer was valid and successfully parsed; false otherwise.
     */
    bool HandleItemOwnership(std::span<const uint8_t> buffer);
}
