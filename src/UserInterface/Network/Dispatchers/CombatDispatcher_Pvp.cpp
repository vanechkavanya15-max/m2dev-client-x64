#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"

#include <cstring>
#include <span>

namespace Network
{
    /**
     * @brief Processes the PVP packet to update character PVP statuses and relationships.
     * 
     * Extracts source and destination Virtual IDs (VIDs) and updates PVP keys and
     * duel states based on the specific mode provided by the server.
     * 
     * @param payload The raw buffer containing the PVP packet data.
     * @return EterBase::PacketResult<void> representing success or specific packet error.
     */
    EterBase::PacketResult<void> HandlePvpPacket(std::span<const uint8_t> payload)
    {
        if (payload.size_bytes() < sizeof(TPacketGCPVP))
        {
            EterBase::ModernLogger::Error("CombatDispatcher_Pvp: Buffer underflow. Expected {} bytes, got {}.", sizeof(TPacketGCPVP), payload.size_bytes());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCPVP pvpPacket;
        std::memcpy(&pvpPacket, payload.data(), sizeof(TPacketGCPVP));

        CPythonCharacterManager& chrMgr = CPythonCharacterManager::Instance();
        CPythonPlayer& player = CPythonPlayer::Instance();

        EterBase::EntityId sourceId(pvpPacket.dwVIDSrc);
        EterBase::EntityId targetId(pvpPacket.dwVIDDst);

        switch (pvpPacket.bMode)
        {
            case PVP_MODE_AGREE:
                chrMgr.RemovePVPKey(sourceId.value(), targetId.value());

                if (player.IsMainCharacterIndex(targetId.value()))
                    player.RememberChallengeInstance(sourceId.value());

                if (player.IsMainCharacterIndex(sourceId.value()))
                    player.RememberCantFightInstance(targetId.value());
                break;

            case PVP_MODE_REVENGE:
                chrMgr.RemovePVPKey(sourceId.value(), targetId.value());

                if (player.IsMainCharacterIndex(targetId.value()))
                    player.RememberRevengeInstance(sourceId.value());

                if (player.IsMainCharacterIndex(sourceId.value()))
                    player.RememberCantFightInstance(targetId.value());
                break;

            case PVP_MODE_FIGHT:
                chrMgr.InsertPVPKey(sourceId.value(), targetId.value());
                player.ForgetInstance(sourceId.value());
                player.ForgetInstance(targetId.value());
                break;

            case PVP_MODE_NONE:
                chrMgr.RemovePVPKey(sourceId.value(), targetId.value());
                player.ForgetInstance(sourceId.value());
                player.ForgetInstance(targetId.value());
                break;

            default:
                EterBase::ModernLogger::Warning("CombatDispatcher_Pvp: Received unknown PVP mode: {}", pvpPacket.bMode);
                break;
        }

        // Publish event instead of direct UI call
        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::TargetBoardRefreshEvent(sourceId.value()));
        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::TargetBoardRefreshEvent(targetId.value()));

        EterBase::ModernLogger::Debug("CombatDispatcher_Pvp: Successfully processed PVP packet for Source: {} and Target: {}.", sourceId.value(), targetId.value());

        return {};
    }
}
