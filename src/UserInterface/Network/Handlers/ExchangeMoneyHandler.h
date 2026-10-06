/**
 * @file ExchangeMoneyHandler.h
 * @brief Modern C++23 handler for trading/exchange money (ELK) packets.
 */

#pragma once

#include <cstdint>
#include <optional>
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"

namespace UserInterface::Network::Handlers {

/**
 * @brief Event triggered to request a refresh of the exchange money UI.
 * 
 * Emitted when gold (ELK) is added/changed in the exchange window.
 */
struct ExchangeMoneyUpdateEvent : public UserInterface::Core::IEvent {
    bool isMe;
    uint32_t amount;

    /**
     * @brief Constructs the exchange money update event.
     * @param isMe True if the gold change applies to the local player, false for target.
     * @param amount The new gold amount.
     */
    ExchangeMoneyUpdateEvent(bool isMe, uint32_t amount) : isMe(isMe), amount(amount) {}
};

/**
 * @class ExchangeMoneyHandler
 * @brief Handles network packets related to the exchange (trade) gold updates.
 *
 * This class processes ELK_ADD subheaders of TPacketGCExchange and
 * updates the CPythonExchange memory state. It adheres to Single Responsibility
 * and Zero-Conflict by separating network packet parsing from the GUI notifications
 * and avoiding modifications to existing handler files.
 */
class ExchangeMoneyHandler
{
public:
    /**
     * @brief Processes an incoming exchange money packet.
     * @param packet The GC exchange packet payload.
     * @return EterBase::PacketResult<void> representing success or a specific packet error.
     */
    static EterBase::PacketResult<void> HandlePacket(const TPacketGCExchange& packet);
};

} // namespace UserInterface::Network::Handlers
