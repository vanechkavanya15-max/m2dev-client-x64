#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../Packet.h"

namespace Network::Handlers
{
    /**
     * @brief Event published when a shop sign packet is processed.
     * 
     * This event replaces direct Python UI calls, allowing the UI 
     * to react to the appearance or disappearance of a private shop sign.
     */
    struct ShopSignEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId targetId;  /**< The virtual ID of the player displaying the sign. */
        std::string sign;             /**< The text of the shop sign. Empty if the shop is closed. */

        /**
         * @brief Constructs a new ShopSignEvent.
         * 
         * @param targetId The entity ID of the shop owner.
         * @param sign The sign text.
         */
        ShopSignEvent(EterBase::EntityId targetId, std::string_view sign)
            : targetId(targetId), sign(sign) {}
    };

    /**
     * @brief Handles incoming network packets related to the private shop sign.
     * 
     * This class parses the raw binary buffers from the network stream
     * and updates the C++ memory state, publishing an event to the EventBus
     * instead of directly calling Python GUI functions.
     */
    class ShopSignHandler
    {
    public:
        /**
         * @brief Processes the shop sign packet.
         * 
         * @param buffer The binary span containing the packet payload.
         * @return EterBase::PacketResult<void> Success if parsed correctly, or an error code on failure.
         */
        static EterBase::PacketResult<void> Process(std::span<const uint8_t> buffer);
    };
}
