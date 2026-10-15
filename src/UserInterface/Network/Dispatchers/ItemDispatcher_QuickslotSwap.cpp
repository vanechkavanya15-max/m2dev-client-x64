#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events/QuickslotEvents.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include <cstdint>
#include <span>

namespace UserInterface::Network {

/**
 * @brief Handles the TPacketGCQuickSlotSwap network packet.
 * 
 * Validates the packet size and slot boundaries, then triggers an event 
 * to notify the UI to refresh the quickslot visual representation.
 * 
 * @param payload The binary payload of the incoming network packet.
 * @return EterBase::PacketResult<void> Success or packet parsing error.
 */
EterBase::PacketResult<void> DispatchQuickSlotSwap(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCQuickSlotSwap)) {
        EterBase::ModernLogger::Error("DispatchQuickSlotSwap: Buffer underflow (expected {}, got {})",
                                      sizeof(TPacketGCQuickSlotSwap), payload.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCQuickSlotSwap*>(payload.data());
    
    // Validate slot positions based on quickslot limitations
    // QUICKSLOT_MAX_NUM is defined in Packet.h as 36.
    if (packet->pos >= QUICKSLOT_MAX_NUM || packet->change_pos >= QUICKSLOT_MAX_NUM) {
         EterBase::ModernLogger::Error("DispatchQuickSlotSwap: Invalid slot positions (pos: {}, change_pos: {})",
                                       packet->pos, packet->change_pos);
         return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("DispatchQuickSlotSwap: Swapping quickslot {} with {}", 
                                 packet->pos, packet->change_pos);
    
    Core::EventBus::GetInstance().Publish(Core::Events::QuickslotSwapEvent{
        EterBase::ItemSlot{static_cast<uint16_t>(packet->pos)}, 
        EterBase::ItemSlot{static_cast<uint16_t>(packet->change_pos)}
    });

    return {};
}

} // namespace UserInterface::Network
