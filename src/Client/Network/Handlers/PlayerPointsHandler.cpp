#include "StdAfx.h"
#include "PlayerPointsHandler.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>

namespace Network::Handlers
{
    EterBase::PacketResult<void> PlayerPointsHandler::HandlePoints(std::span<const uint8_t> payload, UserInterface::Services::IPlayerStatsService& statsService)
    {
        if (payload.size() < sizeof(TPacketGCPoints))
        {
            EterBase::ModernLogger::Error(
                "PlayerPointsHandler: Buffer underflow (expected: {}, got: {})", 
                sizeof(TPacketGCPoints), payload.size()
            );
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCPoints packet;
        std::memcpy(&packet, payload.data(), sizeof(TPacketGCPoints));

        // Prevent division by zero for max stats
        int32_t maxHp = packet.points[POINT_MAX_HP];
        int32_t maxSp = packet.points[POINT_MAX_SP];

        if (maxHp <= 0)
        {
            EterBase::ModernLogger::Warning("PlayerPointsHandler: Received POINT_MAX_HP <= 0, defaulting to 1 to prevent division by zero.");
            maxHp = 1;
        }

        if (maxSp <= 0)
        {
            EterBase::ModernLogger::Warning("PlayerPointsHandler: Received POINT_MAX_SP <= 0, defaulting to 1 to prevent division by zero.");
            maxSp = 1;
        }

        statsService.SetPoint(POINT_HP, packet.points[POINT_HP]);
        statsService.SetPoint(POINT_MAX_HP, maxHp);
        statsService.SetPoint(POINT_SP, packet.points[POINT_SP]);
        statsService.SetPoint(POINT_MAX_SP, maxSp);
        statsService.SetPoint(POINT_EXP, packet.points[POINT_EXP]);
        statsService.SetPoint(POINT_GOLD, packet.points[POINT_GOLD]);

        EterBase::ModernLogger::Info("PlayerPointsHandler: Successfully processed GC::POINTS.");

        return {};
    }
}
