#include "SafeboxSyncPacketHandler.h"
#include "../Protocol/Protocol.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleSafeboxSet(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleSafeboxSet buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemSet), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

    Gameplay::ItemData itemData{
        .vnum = EterBase::ItemVnum(packet->vnum),
        .count = packet->count,
        .size = {1, 1} // Domyslny rozmiar ze wzgledu na brak w pakiecie
    };

    auto result = inventoryDomain.SetItem(Gameplay::InventoryWindow::SafeBox, EterBase::ItemSlot(packet->pos.cell), itemData);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: Failed to set item in safebox (Slot: {}): {}", 
            packet->pos.cell, EterBase::ToString(result.error()));
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Successfully set item (Vnum: {}, Count: {}) in safebox slot {}", 
        packet->vnum, packet->count, packet->pos.cell);

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleSafeboxDel(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) {
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleSafeboxDel buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemDel), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());

    auto result = inventoryDomain.RemoveItem(Gameplay::InventoryWindow::SafeBox, EterBase::ItemSlot(packet->pos.cell));

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: Failed to remove item from safebox (Slot: {}): {}", 
            packet->pos.cell, EterBase::ToString(result.error()));
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Successfully removed item from safebox slot {}", packet->pos.cell);

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleSafeboxSize(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCSafeboxSize)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleSafeboxSize buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCSafeboxSize), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCSafeboxSize*>(buffer.data());
    
    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Received new safebox size {}", packet->bSize);
    Core::EventBus::GetInstance().Publish(SafeboxSyncSizeEvent{packet->bSize});

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleSafeboxWrongPassword(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCSafeboxWrongPassword)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleSafeboxWrongPassword buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCSafeboxWrongPassword), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Wrong password event received");
    Core::EventBus::GetInstance().Publish(SafeboxSyncWrongPasswordEvent{});

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleSafeboxMoneyChange(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCSafeboxMoneyChange)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleSafeboxMoneyChange buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCSafeboxMoneyChange), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCSafeboxMoneyChange*>(buffer.data());

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Money changed to {}", packet->lMoney);
    Core::EventBus::GetInstance().Publish(SafeboxSyncMoneyChangeEvent{packet->lMoney});

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleMallSet(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleMallSet buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemSet), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

    Gameplay::ItemData itemData{
        .vnum = EterBase::ItemVnum(packet->vnum),
        .count = packet->count,
        .size = {1, 1}
    };

    auto result = inventoryDomain.SetItem(Gameplay::InventoryWindow::Mall, EterBase::ItemSlot(packet->pos.cell), itemData);

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: Failed to set item in mall (Slot: {}): {}", 
            packet->pos.cell, EterBase::ToString(result.error()));
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Successfully set item (Vnum: {}, Count: {}) in mall slot {}", 
        packet->vnum, packet->count, packet->pos.cell);

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleMallDel(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain) {
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleMallDel buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemDel), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());

    auto result = inventoryDomain.RemoveItem(Gameplay::InventoryWindow::Mall, EterBase::ItemSlot(packet->pos.cell));

    if (!result.has_value()) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: Failed to remove item from mall (Slot: {}): {}", 
            packet->pos.cell, EterBase::ToString(result.error()));
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Successfully removed item from mall slot {}", packet->pos.cell);

    return {};
}

EterBase::PacketResult<void> SafeboxSyncPacketHandler::HandleMallOpen(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCMallOpen)) {
        EterBase::ModernLogger::Error("SafeboxSyncPacketHandler: HandleMallOpen buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCMallOpen), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCMallOpen*>(buffer.data());
    
    EterBase::ModernLogger::Info("SafeboxSyncPacketHandler: Mall opened with size {}", packet->bSize);
    Core::EventBus::GetInstance().Publish(MallSyncOpenEvent{packet->bSize});

    return {};
}

} // namespace Client::Network::Handlers
