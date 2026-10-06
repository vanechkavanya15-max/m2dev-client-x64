#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <span>
#include <cstdint>

namespace UserInterface::Core {

/**
 * @brief Internal event emitted when a STUN packet is received for an actor.
 * 
 * Used to notify the Actor system to apply stun or faint state (Dead) depending
 * on the target without coupling directly to CPythonCharacterManager.
 */
struct ActorStunnedEvent : public IEvent {
    EterBase::EntityId entityId;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the stunned actor.
     */
    explicit ActorStunnedEvent(EterBase::EntityId id) : entityId(id) {}
};

} // namespace UserInterface::Core

namespace UserInterface::Network {

/**
 * @brief Processes the GC_STUN packet to apply the stun/faint state to an actor.
 * 
 * Parses the packet, extracts the target VID, and publishes an ActorStunnedEvent.
 * 
 * @param payload Binary view of the STUN packet.
 * @return PacketResult<void> Returns success or a strongly typed error (e.g., BufferUnderflow).
 */
[[nodiscard]] EterBase::PacketResult<void> ProcessActorStunPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCStun)) {
        EterBase::ModernLogger::Error("ProcessActorStunPacket: Payload too small. Expected {}, got {}",
                                      sizeof(TPacketGCStun), payload.size());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCStun*>(payload.data());
    const EterBase::EntityId targetId{packet->vid};

    EterBase::ModernLogger::Debug("ProcessActorStunPacket: Received stun packet for EntityId={}", targetId);

    // Decouple logic from UI by publishing the event to the EventBus
    UserInterface::Core::EventBus::GetInstance().Publish(
        UserInterface::Core::ActorStunnedEvent{targetId}
    );

    return {};
}

} // namespace UserInterface::Network
