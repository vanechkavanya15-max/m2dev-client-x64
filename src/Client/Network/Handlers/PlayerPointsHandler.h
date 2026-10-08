#pragma once

#include "StdAfx.h"
#include <cstdint>
#include <span>

#include "../../../EterBase/Result.h"
#include "../Protocol/Protocol.h"
#include "Client/Gameplay/IPlayerStatsService.h"

namespace Network::Handlers
{
    /**
     * @brief Handler for GC::POINTS packet.
     * Decodes player points (HP, SP, EXP, Yang) and updates the IPlayerStatsService safely.
     */
    class PlayerPointsHandler
    {
    public:
        PlayerPointsHandler() = default;
        ~PlayerPointsHandler() = default;

        PlayerPointsHandler(const PlayerPointsHandler&) = delete;
        PlayerPointsHandler& operator=(const PlayerPointsHandler&) = delete;

        /**
         * @brief Processes the TPacketGCPoints packet.
         * 
         * @param payload The network buffer containing the packet.
         * @param statsService The service to update player stats.
         * @return EterBase::PacketResult<void> Success or PacketError.
         */
        static EterBase::PacketResult<void> HandlePoints(std::span<const uint8_t> payload, Client::Gameplay::IPlayerStatsService& statsService);
    };
}
