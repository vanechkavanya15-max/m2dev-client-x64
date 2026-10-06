#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

class CNetworkStream; // Forward declaration

namespace UserInterface::Core {
    /**
     * @brief Event emitted when a script button packet is successfully sent to the server.
     * 
     * Allows UI components to decouple from the network layer.
     */
    struct ScriptButtonSentEvent : public IEvent {
        uint32_t buttonIndex;

        /**
         * @brief Constructs the event with the given button index.
         * @param buttonIndex The index of the script button clicked.
         */
        explicit ScriptButtonSentEvent(uint32_t buttonIndex) : buttonIndex(buttonIndex) {}
    };
} // namespace UserInterface::Core

namespace Network::Senders {
    /**
     * @brief Formats and sends a script button interaction packet to the server.
     * 
     * Applies C++23 guidelines, replaces boolean returns with expected, and correctly aligns
     * structure packing prior to network transmission.
     * 
     * @param buttonIndex The index of the script button being clicked.
     * @param networkStream Pointer to the network stream used to send the payload.
     * @return EterBase::PacketResult<void> representing success or strict failure details.
     */
    EterBase::PacketResult<void> SendScriptButtonPacket(uint32_t buttonIndex, CNetworkStream* networkStream);
} // namespace Network::Senders
