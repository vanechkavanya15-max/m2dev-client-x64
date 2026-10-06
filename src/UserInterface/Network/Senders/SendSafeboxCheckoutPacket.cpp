#include "StdAfx.h"
#include "SendSafeboxCheckoutPacket.h"
#include "../../../EterLib/NetStream.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <span>

namespace Network::Senders
{
    EterBase::PacketResult<void> SendSafeboxCheckoutHandler::SendSafeboxCheckout(EterBase::ItemSlot safeBoxSlot, const TItemPos& inventoryPos, CNetworkStream* networkStream)
    {
        if (!networkStream)
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckout failed: null network stream");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        TPacketCGSafeboxCheckout packet;
        
        std::memset(&packet, 0, sizeof(packet));
        packet.header = CG::SAFEBOX_CHECKOUT;
        packet.length = sizeof(packet);
        packet.bSafePos = static_cast<uint8_t>(safeBoxSlot.get());
        packet.ItemPos = inventoryPos;

        std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
        {
            EterBase::ModernLogger::Error("SendSafeboxCheckout failed to send packet data to stream");
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Info("SendSafeboxCheckout success: safeboxSlot={}, inventoryPos.window={}, inventoryPos.cell={}", 
            safeBoxSlot.get(), inventoryPos.window_type, inventoryPos.cell);
            
        return {};
    }
}
