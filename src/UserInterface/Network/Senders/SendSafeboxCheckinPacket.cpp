#include "../../StdAfx.h"
#include "SendSafeboxCheckinPacket.h"

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Packet.h"
#include "../../GameType.h"
#include "../../../EterLib/NetStream.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events/SafeboxEvents.h"

#include <span>

EterBase::PacketResult<void> SendSafeboxCheckinPacketHandler::Send(CNetworkStream* networkStream, uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot)
{
    if (!networkStream)
    {
        EterBase::ModernLogger::Error("SendSafeboxCheckinPacketHandler::Send - Network stream is null.");
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    TPacketCGSafeboxCheckin packet{};
    // Use the CG namespace for client-to-game packet headers
    packet.header = CG::SAFEBOX_CHECKIN;
    packet.length = static_cast<uint16_t>(sizeof(packet));
    packet.bSafePos = safeboxSlot;
    
    // Convert strong type to network expected format (TItemPos)
    packet.ItemPos = TItemPos(INVENTORY, inventorySlot.value());

    std::span<const uint8_t> packetSpan{reinterpret_cast<const uint8_t*>(&packet), sizeof(packet)};

    if (!networkStream->Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
    {
        EterBase::ModernLogger::Error("SendSafeboxCheckinPacketHandler::Send - Failed to send payload (safeboxSlot: {}, inventorySlot: {}).", 
                                      safeboxSlot, inventorySlot.value());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    EterBase::ModernLogger::Info("SendSafeboxCheckinPacketHandler::Send - Checkin successful (safeboxSlot: {}, inventorySlot: {}).", 
                                 safeboxSlot, inventorySlot.value());
                                 
    // Emit an event as per architecture rules
    UserInterface::Core::EventBus::GetInstance().Publish(Core::Events::SafeboxCheckinSentEvent{safeboxSlot, inventorySlot.value()});
                                 
    return {};
}
