#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

class CPythonNetworkStream;

namespace Network::Handlers {

/**
 * @brief Event emitted when an item is set in a specific inventory or equipment slot.
 * 
 * Used to decouple the UI from the network layer. The UI can listen to this event 
 * and update itself accordingly without direct invocation from the network handler.
 */
struct ItemSetModernEvent : public UserInterface::Core::IEvent {
    uint8_t windowType;
    EterBase::ItemSlot slot;
    EterBase::ItemVnum vnum;
    uint8_t count;
    uint32_t flags;
    uint32_t antiFlags;
    bool highlight;

    /**
     * @brief Constructs the ItemSetModernEvent.
     * @param windowType The inventory window type.
     * @param slot The slot where the item is set.
     * @param vnum The virtual number (ID) of the item.
     * @param count The amount/count of the item.
     * @param flags The flags associated with the item.
     * @param antiFlags The anti-flags associated with the item.
     * @param highlight Whether the item should be highlighted.
     */
    ItemSetModernEvent(uint8_t windowType, EterBase::ItemSlot slot, EterBase::ItemVnum vnum, uint8_t count, uint32_t flags, uint32_t antiFlags, bool highlight)
        : windowType(windowType), slot(slot), vnum(vnum), count(count), flags(flags), antiFlags(antiFlags), highlight(highlight) {}
};

/**
 * @brief Modern handler for the Item Set network packet.
 */
class ItemSetModernHandler {
public:
    /**
     * @brief Handles the item set packet from the network stream.
     * @param stream The network stream to read the packet from.
     * @return A PacketResult indicating success or a specific PacketError.
     */
    static EterBase::PacketResult<void> Handle(CPythonNetworkStream& stream);
};

} // namespace Network::Handlers
