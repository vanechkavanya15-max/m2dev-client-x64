#include "SafeboxHandler.h"
#include "UserInterface/Packet.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> HandleSafeboxSet(std::span<const uint8_t> buffer, Client::Gameplay::SafeBox& safeBox) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        EterBase::ModernLogger::Error("HandleSafeboxSet: Buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemSet), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

    EterBase::ItemSlot slot{packet->pos.cell};
    Client::Gameplay::SafeBoxItem item{EterBase::ItemVnum{packet->vnum}, packet->count};

    auto result = safeBox.SetItem(slot, item);
    if (!result) {
        EterBase::ModernLogger::Error("HandleSafeboxSet: Failed to set item in safebox (Slot: {})", slot.get());
    } else {
        EterBase::ModernLogger::Info("HandleSafeboxSet: Successfully set item (Vnum: {}, Count: {}) in safebox slot {}", 
            item.vnum.get(), item.count, slot.get());
    }

    return {};
}

EterBase::PacketResult<void> HandleSafeboxDel(std::span<const uint8_t> buffer, Client::Gameplay::SafeBox& safeBox) {
    if (buffer.size() < sizeof(TPacketGCItemDel)) {
        EterBase::ModernLogger::Error("HandleSafeboxDel: Buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCItemDel), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());
    EterBase::ItemSlot slot{packet->pos.cell};

    auto result = safeBox.RemoveItem(slot);
    if (!result) {
        EterBase::ModernLogger::Error("HandleSafeboxDel: Failed to remove item from safebox (Slot: {})", slot.get());
    } else {
        EterBase::ModernLogger::Info("HandleSafeboxDel: Successfully removed item from safebox slot {}", slot.get());
    }

    return {};
}

EterBase::PacketResult<void> HandleSafeboxSize(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCSafeboxSize)) {
        EterBase::ModernLogger::Error("HandleSafeboxSize: Buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCSafeboxSize), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCSafeboxSize*>(buffer.data());
    
    EterBase::ModernLogger::Info("HandleSafeboxSize: Received new safebox size {}", packet->bSize);
    UserInterface::Core::EventBus::GetInstance().Publish(SafeBoxSizeChangedEvent{packet->bSize});

    return {};
}

EterBase::PacketResult<void> HandleSafeboxWrongPassword(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCSafeboxWrongPassword)) {
        EterBase::ModernLogger::Error("HandleSafeboxWrongPassword: Buffer underflow (expected {}, got {})", 
            sizeof(TPacketGCSafeboxWrongPassword), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    EterBase::ModernLogger::Info("HandleSafeboxWrongPassword: Wrong password event received");
    UserInterface::Core::EventBus::GetInstance().Publish(SafeBoxWrongPasswordEvent{});

    return {};
}

} // namespace Client::Network::Handlers
