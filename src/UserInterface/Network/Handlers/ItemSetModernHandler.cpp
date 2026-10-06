#include "../../StdAfx.h"
#include "ItemSetModernHandler.h"
#include "../../PythonNetworkStream.h"
#include "../../AbstractPlayer.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include <optional>
#include <algorithm>

namespace Network::Handlers {

EterBase::PacketResult<void> ItemSetModernHandler::Handle(CPythonNetworkStream& stream) {
    TPacketGCItemSet packet;
    if (!stream.Recv(sizeof(packet), &packet)) {
        EterBase::ModernLogger::Error("ItemSetModernHandler: Failed to read TPacketGCItemSet from stream.");
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TItemData itemData{};
    itemData.vnum = packet.vnum;
    itemData.count = packet.count;
    itemData.flags = packet.flags;
    itemData.anti_flags = packet.anti_flags;

    // Use std::copy for cleaner code instead of manual loops
    std::copy(std::begin(packet.alSockets), std::end(packet.alSockets), std::begin(itemData.alSockets));
    std::copy(std::begin(packet.aAttr), std::end(packet.aAttr), std::begin(itemData.aAttr));

    auto updatePlayerState = [&]() -> std::optional<bool> {
        IAbstractPlayer& player = IAbstractPlayer::GetSingleton();
        player.SetItemData(packet.pos, itemData);
        return true;
    };

    updatePlayerState().and_then([&](bool /*success*/) -> std::optional<bool> {
        ItemSetModernEvent event(
            packet.pos.window_type,
            EterBase::ItemSlot(static_cast<uint16_t>(packet.pos.cell)),
            EterBase::ItemVnum(packet.vnum),
            packet.count,
            packet.flags,
            packet.anti_flags,
            static_cast<bool>(packet.highlight)
        );
        UserInterface::Core::EventBus::GetInstance().Publish(event);
        return true;
    }).value_or(false);

    EterBase::ModernLogger::Debug(
        "ItemSetModernHandler: Set item vnum: {}, slot: {}, count: {}, flags: {}, anti_flags: {}, highlight: {}", 
        packet.vnum, packet.pos.cell, packet.count, packet.flags, packet.anti_flags, packet.highlight
    );

    return {};
}

} // namespace Network::Handlers
