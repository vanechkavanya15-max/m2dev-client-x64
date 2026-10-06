#include "StdAfx.h"
#include "PointsHandler.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/LogModern.h"

/**
 * @file PointsHandler.cpp
 * @brief Implementation of full points synchronization.
 * 
 * Replaces direct calls to PythonNetworkStream variables and Python function invocation
 * with EventBus-based communication and C++23 std::expected error handling.
 */

EterBase::PacketResult<void> PointsHandler::HandlePoints(const TPacketGCPoints& packet)
{
    auto& player = CPythonPlayer::Instance();

    // Iterate through all points and securely update the central state.
    for (uint32_t i = 0; i < POINT_MAX_NUM; ++i)
    {
        player.SetStatus(i, packet.points[i]);
    }

    // Emit an event for UI and other decoupled components to react to the updated points
    // (HP, MP, EXP, Stamina, Gold). The decoupled receivers can query CPythonPlayer
    // for specific values to avoid fat event payloads.
    PlayerPointsUpdateEvent updateEvent;
    UserInterface::Core::EventBus::GetInstance().Publish(updateEvent);

    return {};
}
