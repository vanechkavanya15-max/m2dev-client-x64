#pragma once

#include <cstdint>
#include <span>
#include "EterBase/Result.h"

namespace Client::Network::Handlers {

/**
 * @class TargetHpPacketHandler
 * @brief Handler for target HP update network packets.
 * 
 * Ten komponent jest odpowiedzialny za odbieranie i parsowanie
 * pakietow aktualizujacych pasek zdrowia zaznaczonego wroga.
 */
class TargetHpPacketHandler {
public:
    /**
     * @brief Parses and handles the TargetHP packet payload.
     * @param payload Span containing the raw network data.
     * @return PacketResult<void> indicating success or an error code.
     */
    static EterBase::PacketResult<void> HandleTargetHpUpdate(std::span<const uint8_t> payload) noexcept;
};

} // namespace Client::Network::Handlers
