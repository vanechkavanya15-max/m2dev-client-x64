#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../PythonSafeBox.h"
#include <optional>
#include <algorithm>

namespace Network::Dispatchers {

/**
 * @brief Modern event to decouple the UI from the Mall Set packet logic.
 */
struct MallItemSetEvent : public UserInterface::Core::IEvent {
    EterBase::ItemSlot slot;
    EterBase::ItemVnum vnum;
    uint8_t count;
    uint32_t flags;
    uint32_t antiFlags;

    MallItemSetEvent(EterBase::ItemSlot slot, EterBase::ItemVnum vnum, uint8_t count, uint32_t flags, uint32_t antiFlags)
        : slot(slot), vnum(vnum), count(count), flags(flags), antiFlags(antiFlags) {}
};

/**
 * @brief Handles the TPacketGCItemSet network packet for the ItemShop (Mall).
 * @param buffer Raw byte span containing the packet data.
 * @return PacketResult indicating success or an error code.
 */
EterBase::PacketResult<void> HandleMallItemSet(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCItemSet)) {
        EterBase::ModernLogger::Error("HandleMallItemSet: Buffer too small ({} < {})", buffer.size(), sizeof(TPacketGCItemSet));
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());

    TItemData itemData{};
    itemData.vnum = packet->vnum;
    itemData.count = packet->count;
    itemData.flags = packet->flags;
    itemData.anti_flags = packet->anti_flags;

    std::copy(std::begin(packet->alSockets), std::end(packet->alSockets), std::begin(itemData.alSockets));
    std::copy(std::begin(packet->aAttr), std::end(packet->aAttr), std::begin(itemData.aAttr));

    CPythonSafeBox::Instance().SetMallItemData(packet->pos.cell, itemData);

    MallItemSetEvent event(
        EterBase::ItemSlot(static_cast<uint16_t>(packet->pos.cell)),
        EterBase::ItemVnum(packet->vnum),
        packet->count,
        packet->flags,
        packet->anti_flags
    );
    UserInterface::Core::EventBus::GetInstance().Publish(event);

    EterBase::ModernLogger::Debug(
        "HandleMallItemSet: Set mall item vnum: {}, slot: {}, count: {}",
        packet->vnum, packet->pos.cell, packet->count
    );

    return {};
}

} // namespace Network::Dispatchers
