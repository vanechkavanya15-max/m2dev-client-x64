#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterLib/ControlPackets.h"

namespace Network::Handlers
{
    /**
     * @brief Zglasza zmiane fazy gry (Login, Select, Loading, Game).
     * @param buffer Bufor z danymi pakietu reprezentujacymi TPacketGCPhase.
     * @return EterBase::PacketResult<void> wskazujacy sukces lub kod bledu PacketError.
     */
    EterBase::PacketResult<void> ProcessPhaseChangePacket(std::span<const uint8_t> buffer);
}
