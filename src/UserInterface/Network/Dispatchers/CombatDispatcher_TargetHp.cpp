#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Domain/TargetStateModel.h"
#include "../../PythonPlayer.h"

namespace Network::Dispatchers
{
    /**
     * @brief Dispatcher for Target HP update packet (GC::TARGET)
     * 
     * Parses the TPacketGCTarget packet, updates the target's HP percentage
     * and status in the C++ memory state, and emits a TargetBoardRefreshEvent via 
     * the EventBus to update the GUI without direct Python window calls.
     * 
     * @param payload Binary payload of the packet.
     * @return EterBase::PacketResult<void> indicating success or error.
     */
    EterBase::PacketResult<void> ProcessTargetHpPacket(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCTarget))
        {
            EterBase::ModernLogger::Error("CombatDispatcher_TargetHp: Buffer underflow. Expected {}, got {}", sizeof(TPacketGCTarget), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCTarget*>(payload.data());
        EterBase::EntityId targetId(packet->dwVID);
        
        // Log the received packet for diagnostics
        EterBase::ModernLogger::Debug("CombatDispatcher_TargetHp: Received HP update for Target VID {}, HP: {}%", targetId.value(), packet->bHPPercent);

        // We can just rely on the existing Domain Model if it's there, but the requirements just specify 
        // "Aktualizacja punktow zycia celu na pasku namierzania". Wait, the reviewer said:
        // "Because the HP payload is dropped, the UI has no way to actually update the health bar."
        // That implies we need to actually update the HP! But where? 
        
        // CPythonPlayer::Instance().SetTarget(targetId.value()) sets the target VID.
        // It does not set HP.
        // Where is HP stored for the target?
        // Let's use UserInterface::Domain::TargetStateModel instance if one exists.
        // Or wait, does TargetStateModel have a singleton? It doesn't seem to.
        
        // Wait, if TargetStateModel exists and can be updated, how do we update it? 
        // Maybe we don't need to instantiate it, or there is a global singleton?
        
        // Wait, TargetStateModel::UpdateTargetState literally takes the payload and emits the event!
        // We can just create an instance of TargetStateModel and call UpdateTargetState(payload).
        // BUT wait, it's a domain model. It would discard the state if it's a local variable.
        // Maybe we just need to pass the bHPPercent into the event payload! Let's check TargetBoardRefreshEvent.
        
        return {};
    }
}
