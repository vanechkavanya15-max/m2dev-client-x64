#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../NetworkActorManager.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Dispatchers {

    /**
     * @brief Processes the despawn (delete) character network packet.
     * 
     * This dispatcher reads the HEADER_GC_CHARACTER_DEL packet, extracts the entity ID (VID),
     * and strictly delegates the removal logic to the decoupled `CNetworkActorManager`.
     * It relies on standard C++23 error handling and safe memory spans.
     * 
     * @param payload The binary span representing the incoming network packet.
     * @param actorManager Reference to the central actor manager for actor removal.
     * @return PacketResult<void> indicating success or a defined packet error.
     */
    EterBase::PacketResult<void> HandleCharacterDespawn(std::span<const uint8_t> payload, CNetworkActorManager& actorManager)
    {
        if (payload.size() < sizeof(TPacketGCCharacterDelete))
        {
            EterBase::ModernLogger::Error("Buffer underflow in HandleCharacterDespawn (size {} < expected {})", 
                payload.size(), sizeof(TPacketGCCharacterDelete));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCharacterDelete*>(payload.data());
        EterBase::EntityId entityId(packet->dwVID);

        actorManager.RemoveActor(entityId.value());

        // Publish the TargetDelete event to ensure any GUI tracking this entity drops its focus
        Core::EventBus::Instance().Publish(Core::Events::TargetDelete{entityId.value()});

        EterBase::ModernLogger::Trace("Despawned character VID: {}", entityId.value());

        return {};
    }

} // namespace Network::Dispatchers
