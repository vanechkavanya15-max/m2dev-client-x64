#include "QuickslotSyncPacketHandler.h"
#include "../../Gameplay/QuickslotDomain.h"
#include "../Protocol/Protocol.h"
#include "../../../EterBase/LogModern.h"
#include "../Protocol/ProtocolTypes.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> QuickslotSyncPacketHandler::HandleAdd(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept
{
    if (buffer.size() < sizeof(TPacketGCQuickSlotAdd)) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleAdd: Buffer size too small: {} < {}", buffer.size(), sizeof(TPacketGCQuickSlotAdd));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCQuickSlotAdd*>(buffer.data());

    Client::Gameplay::QuickslotItem item{packet->slot.Type, packet->slot.Position};
    auto result = quickslotDomain.SetSlot(packet->pos, item);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleAdd: Failed to set slot at pos: {}, error: {}", 
            packet->pos, EterBase::ToString(result.error()));
        
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

EterBase::PacketResult<void> QuickslotSyncPacketHandler::HandleDel(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept
{
    if (buffer.size() < sizeof(TPacketGCQuickSlotDel)) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleDel: Buffer size too small: {} < {}", buffer.size(), sizeof(TPacketGCQuickSlotDel));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCQuickSlotDel*>(buffer.data());

    auto result = quickslotDomain.ClearSlot(packet->pos);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleDel: Failed to clear slot at pos: {}, error: {}", 
            packet->pos, EterBase::ToString(result.error()));
        
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

EterBase::PacketResult<void> QuickslotSyncPacketHandler::HandleSwap(std::span<const uint8_t> buffer, Client::Gameplay::QuickslotDomain& quickslotDomain) noexcept
{
    if (buffer.size() < sizeof(TPacketGCQuickSlotSwap)) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleSwap: Buffer size too small: {} < {}", buffer.size(), sizeof(TPacketGCQuickSlotSwap));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCQuickSlotSwap*>(buffer.data());

    auto result = quickslotDomain.SwapSlots(packet->pos, packet->change_pos);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("QuickslotSyncPacketHandler::HandleSwap: Failed to swap slots pos: {} and change_pos: {}, error: {}", 
            packet->pos, packet->change_pos, EterBase::ToString(result.error()));
        
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    return {};
}

} // namespace Client::Network::Handlers
