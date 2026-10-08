#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

class CNetworkStream; // Forward declaration

namespace UserInterface::Network::Senders
{
    /**
     * @brief Event published when an attack packet is successfully sent.
     * 
     * This decouples the network packet sending from UI and game state updates,
     * adhering to Event-Driven and Zero-Conflict architecture.
     */
    struct NetworkAttackSentEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId victimId; ///< The target entity that was attacked.
        uint8_t attackType;          ///< The type of the attack (e.g., motion index).

        /**
         * @brief Constructs the event.
         * @param victimId The target's entity ID.
         * @param attackType The type of attack performed.
         */
        NetworkAttackSentEvent(EterBase::EntityId victimId, uint8_t attackType)
            : victimId(victimId), attackType(attackType) {}
    };

    /**
     * @brief Sender class for dispatching attack packets to the Game Server.
     */
    class AttackSender
    {
    public:
        /**
         * @brief Sends an attack packet to the server securely using modern C++23 standards.
         * 
         * Constructs a strictly-packed network payload and dispatches it through the provided
         * CNetworkStream. Avoids legacy output parameters and returns a strong result type.
         * 
         * @param victimId The unique strong ID of the target being attacked.
         * @param attackType The motion or type identifier of the attack.
         * @param networkStream Pointer to the network stream to transmit the data.
         * @return EterBase::PacketDispatchResult<void> representing success or specific network failure.
         */
        static EterBase::PacketDispatchResult<void> Send(EterBase::EntityId victimId, uint8_t attackType, CNetworkStream* networkStream);
    };
} // namespace UserInterface::Network::Senders
