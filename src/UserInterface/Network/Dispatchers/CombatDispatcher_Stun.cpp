/**
 * @file CombatDispatcher_Stun.cpp
 * @brief Dispatcher for STUN combat state packet (C++23 Modernization).
 * 
 * Implements Zero-Conflict handling of TPacketGCStun.
 * Decouples network layer from Python UI via EventBus.
 */

#include "../../StdAfx.h"
#include "../../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <span>
#include <cstdint>

namespace UserInterface::Core {

/**
 * @brief Internal event emitted when a STUN packet is received for an entity.
 * 
 * Replaces direct calls to CPythonPlayer/CPythonCharacterManager.
 */
struct EntityStunnedEvent : public IEvent {
    EterBase::EntityId entityId;

    /**
     * @brief Constructs the event.
     * @param id The unique identifier of the stunned entity.
     */
    explicit EntityStunnedEvent(EterBase::EntityId id) : entityId(id) {}
};

} // namespace UserInterface::Core

namespace UserInterface::Network {

/**
 * @brief Processes the GC_STUN packet to apply the stun state.
 * 
 * Parses the packet, extracts the target VID, and publishes an EntityStunnedEvent.
 * 
 * @param payload Binary view of the STUN packet.
 * @return PacketResult<void> Returns success or a strongly typed error (e.g., BufferUnderflow).
 */
[[nodiscard]] EterBase::PacketResult<void> DispatchStunState(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCStun)) {
        EterBase::ModernLogger::Error("DispatchStunState: Payload too small. Expected {}, got {}",
                                      sizeof(TPacketGCStun), payload.size());
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCStun*>(payload.data());
    const EterBase::EntityId targetId{packet->vid};

    EterBase::ModernLogger::Debug("DispatchStunState: Received stun packet for EntityId={}", targetId);

    // Decouple logic from UI by publishing the event
    UserInterface::Core::EventBus::GetInstance().Publish(
        UserInterface::Core::EntityStunnedEvent{targetId}
    );

    return {};
}

} // namespace UserInterface::Network
