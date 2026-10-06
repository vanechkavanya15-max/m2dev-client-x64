#include "StdAfx.h"
#include "SendItemMovePacket.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include <cstring>
#include <format>

namespace Core
{
    using EventBus = UserInterface::Core::EventBus;
}

namespace Network
{
    /**
     * @brief Formats and sends an item move packet to the server.
     * 
     * @param sourcePos The original position of the item in the inventory/safebox.
     * @param targetPos The destination position for the item.
     * @param count The number of items to move.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return EterBase::PacketResult<void> Returns expected success or an error if sending fails.
     */
    EterBase::PacketResult<void> SendItemMovePacket(const TItemPos& sourcePos, 
                                                    const TItemPos& targetPos, 
                                                    uint8_t count, 
                                                    const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
#pragma pack(push, 1)
        TPacketCGItemMove movePacket;
#pragma pack(pop)
        
        std::memset(&movePacket, 0, sizeof(movePacket));
        
        movePacket.header = CG::ITEM_MOVE;
        movePacket.length = sizeof(movePacket);
        movePacket.pos = sourcePos;
        movePacket.change_pos = targetPos;
        movePacket.num = count;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&movePacket), sizeof(movePacket));

        if (!sendCallback(packetSpan))
        {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        // Emit an event to the EventBus
        Core::EventBus::GetInstance().Publish(UserInterface::Core::NetworkPacketReceivedEvent(CG::ITEM_MOVE, packetSpan));

        return {};
    }
}
