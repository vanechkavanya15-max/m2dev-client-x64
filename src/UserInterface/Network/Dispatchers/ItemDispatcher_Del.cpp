#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../AbstractPlayer.h"
#include "../../PythonNetworkStream.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace Network::Dispatchers {

/**
 * @brief Event emitted when an item is deleted from a specific inventory or equipment slot.
 * 
 * Used to decouple the UI from the network layer. The UI can listen to this event 
 * and update itself accordingly without direct invocation from the network handler.
 */
struct ItemDelModernEvent : public UserInterface::Core::IEvent {
    uint8_t windowType;
    EterBase::ItemSlot slot;

    /**
     * @brief Constructs the ItemDelModernEvent.
     * @param windowType The inventory window type.
     * @param slot The slot where the item is deleted.
     */
    ItemDelModernEvent(uint8_t windowType, EterBase::ItemSlot slot)
        : windowType(windowType), slot(slot) {}
};

/**
 * @brief Handles the item delete packet from the network stream and clears the slot in memory.
 * 
 * @param stream The network stream to read the packet from.
 * @return EterBase::PacketResult<void> Success or PacketError::BufferUnderflow.
 */
EterBase::PacketResult<void> DispatchItemDel(CPythonNetworkStream& stream) {
    TPacketGCItemDel packet;
    if (!stream.Recv(sizeof(packet), &packet)) {
        EterBase::ModernLogger::Error("ItemDispatcher_Del: Failed to read TPacketGCItemDel from stream.");
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    IAbstractPlayer::GetSingleton().SetItemData(packet.pos, TItemData{});

    ItemDelModernEvent event(packet.pos.window_type, EterBase::ItemSlot(packet.pos.cell));
    UserInterface::Core::EventBus::GetInstance().Publish(event);

    EterBase::ModernLogger::Debug(
        "ItemDispatcher_Del: Cleared item at window_type: {}, slot: {}",
        packet.pos.window_type, packet.pos.cell
    );

    return {};
}

} // namespace Network::Dispatchers
