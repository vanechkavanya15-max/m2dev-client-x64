#include "../StdAfx.h"
#include "IQuickslotService.h"
#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <functional>
#include <span>
#include <cstring>

namespace UserInterface::Services
{
    namespace
    {
        constexpr uint16_t QUICKSLOT_MAX_NUM = 36;

        // Zero-Conflict packed proxy structure for the delete packet
#pragma pack(push, 1)
        struct QuickslotDelPacketProxy
        {
            uint16_t header;
            uint16_t length;
            uint8_t  pos;
        };
#pragma pack(pop)
        static_assert(sizeof(QuickslotDelPacketProxy) == 5, "QuickslotDelPacketProxy size mismatch");
    }

    /**
     * @brief Modern C++23 zero-conflict function to delete a quickslot.
     * 
     * @param slotIndex Strong type representing the quickslot index to delete.
     * @param sendCallback Network callback to dispatch the packet.
     * @return EterBase::PacketResult<void> Returns success or a domain error.
     */
    EterBase::PacketResult<void> SendDeleteQuickslotPacket(
        EterBase::ItemSlot slotIndex,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (slotIndex.value() >= QUICKSLOT_MAX_NUM)
        {
            EterBase::ModernLogger::Error("Failed to delete quickslot: index {} out of bounds", slotIndex.value());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        QuickslotDelPacketProxy packet{};
        packet.header = CG::QUICKSLOT_DEL;
        packet.length = sizeof(QuickslotDelPacketProxy);
        packet.pos = static_cast<uint8_t>(slotIndex.value());

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

        if (!sendCallback(packetSpan))
        {
            EterBase::ModernLogger::Error("Failed to send quickslot delete packet: session closed.");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        Core::EventBus::GetInstance().Publish(
            Core::NetworkPacketReceivedEvent(static_cast<uint8_t>(CG::QUICKSLOT_DEL), packetSpan)
        );

        EterBase::ModernLogger::Info("Successfully sent quickslot delete packet for index {}", slotIndex.value());

        return {};
    }
} // namespace UserInterface::Services
