#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"

/**
 * @brief Exchange accept event triggered when trade readiness changes.
 */
struct ExchangeAcceptEvent : public UserInterface::Core::IEvent {
    bool isInitiator;
    bool isAccepted;

    ExchangeAcceptEvent(bool isInitiator, bool isAccepted)
        : isInitiator(isInitiator), isAccepted(isAccepted) {}
};


/**
 * @class ExchangeAcceptHandler
 * @brief Handles the accept/ready phase of a player exchange.
 * 
 * Responsibilities include taking the packet, extracting the target (self vs victim),
 * updating the PythonExchange memory state, and emitting a UI refresh event.
 */
class ExchangeAcceptHandler
{
public:
    /**
     * @brief Processes an incoming exchange accept packet.
     * @param packet The incoming GC exchange packet.
     * @return PacketResult indicating success or an error code.
     */
    static EterBase::PacketResult<void> HandleAccept(const TPacketGCExchange& packet);
};
