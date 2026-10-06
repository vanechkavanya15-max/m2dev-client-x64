#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"

namespace Network::Handlers
{
    /**
     * @brief Event payload for Lover Info.
     * 
     * This event is published to the EventBus when the lover info packet is processed.
     */
    struct LoverInfoEvent
    {
        std::string name;
        uint8_t lovePoint;
    };

    /**
     * @brief Processes the Lover Info packet.
     * 
     * @param buffer The incoming packet buffer.
     * @return EterBase::PacketResult<void> Returns a success result or an error type if processing fails.
     */
    EterBase::PacketResult<void> ProcessLoverInfo(std::span<const uint8_t> buffer);

    /**
     * @brief Legacy wrapper for Lover Info packet processing.
     * 
     * @param buffer The incoming packet buffer.
     * @return bool True if processing succeeds, false otherwise.
     */
    bool HandleLoverInfo(std::span<const uint8_t> buffer);
}
