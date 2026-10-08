#include "StdAfx.h"
#include "ItemGroundDelHandler.h"
#include "../../../UserInterface/PythonItem.h"
#include <cstring>

namespace {
    UserInterface::Core::EventBus& EventBusInstance()
    {
        return UserInterface::Core::EventBus::GetInstance();
    }
} // namespace

EterBase::PacketResult<void> ItemGroundDelHandler::Handle(std::span<const uint8_t> buffer)
{
    // Step 1: Validate buffer size using C++23 monadic transform
    auto validateSize = [](std::span<const uint8_t> buf) -> EterBase::PacketResult<std::span<const uint8_t>> {
        if (buf.size() < sizeof(ItemGroundDelPacket)) {
            EterBase::ModernLogger::Error("ItemGroundDelHandler: Buffer underflow (size: {}, expected: {})",
                                          buf.size(), sizeof(ItemGroundDelPacket));
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        return buf;
    };

    // Step 2: Extract and parse packet
    auto parsePacket = [](std::span<const uint8_t> buf) -> ItemGroundDelPacket {
        ItemGroundDelPacket packet;
        std::memcpy(&packet, buf.data(), sizeof(ItemGroundDelPacket));
        return packet;
    };

    // Step 3: Handle deletion logic
    auto handleDeletion = [](const ItemGroundDelPacket& packet) -> void {
        EterBase::EntityId dropVid(packet.itemVid);

        // Remove the 3D model from the game world
        CPythonItem::Instance().DeleteItem(dropVid.value());

        // Dispatch event via EventBus to inform UI (e.g., TextTailEngine) to unregister the text
        ItemGroundDelEvent event(dropVid);
        EventBusInstance().Publish(event);

        EterBase::ModernLogger::Debug("ItemGroundDelHandler: Dispatched delete event (VID: {})", packet.itemVid);
    };

    // Execute monadic chain
    return validateSize(buffer)
        .transform(parsePacket)
        .transform(handleDeletion);
}
