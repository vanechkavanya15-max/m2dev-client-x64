#include "StdAfx.h"
#include "ItemGroundAddModernHandler.h"
#include <cstring>

namespace {
    /**
     * @brief Helper to retrieve the EventBus instance according to the established Singleton pattern.
     * @return UserInterface::Core::EventBus& The event bus instance.
     */
    UserInterface::Core::EventBus& EventBusInstance()
    {
        return UserInterface::Core::EventBus::GetInstance();
    }
} // namespace

EterBase::PacketResult<void> ItemGroundAddModernHandler::Handle(std::span<const uint8_t> buffer)
{
    // Step 1: Validate buffer size using C++23 monadic transform
    auto validateSize = [](std::span<const uint8_t> buf) -> EterBase::PacketResult<std::span<const uint8_t>> {
        if (buf.size() < sizeof(ItemGroundAddModernPacket)) {
            EterBase::ModernLogger::Error("ItemGroundAddModernHandler: Buffer underflow (size: {}, expected: {})",
                                          buf.size(), sizeof(ItemGroundAddModernPacket));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }
        return buf;
    };

    // Step 2: Extract and parse packet
    auto parsePacket = [](std::span<const uint8_t> buf) -> ItemGroundAddModernPacket {
        ItemGroundAddModernPacket packet;
        std::memcpy(&packet, buf.data(), sizeof(ItemGroundAddModernPacket));
        return packet;
    };

    // Step 3: Dispatch event via EventBus
    auto dispatchEvent = [](const ItemGroundAddModernPacket& packet) -> void {
        EterBase::EntityId dropVid(packet.id);
        EterBase::ItemVnum itemVnum(packet.vnum);
        
        int32_t x = packet.x;
        // Metin2 legacy coordinate fix: scale Y coordinate by 100 if greater than 10
        int32_t y = (packet.y > 10) ? (packet.y * 100) : packet.y;
        int32_t z = packet.z;

        ItemGroundAddEvent event(dropVid, itemVnum, x, y, z);
        EventBusInstance().Publish(event);

        EterBase::ModernLogger::Debug("ItemGroundAddModernHandler: Dispatched drop event (VID: {}, Vnum: {})",
                                      packet.id, packet.vnum);
    };

    // Execute monadic chain
    return validateSize(buffer)
        .transform(parsePacket)
        .transform(dispatchEvent);
}
