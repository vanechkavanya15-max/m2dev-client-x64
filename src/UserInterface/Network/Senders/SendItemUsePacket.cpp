#include "StdAfx.h"
#include "SendItemUsePacket.h"
#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Senders
{
    /**
     * @brief Implementacja wysylania polecenia uzycia przedmiotu przez gracza (C++23)
     */
    EterBase::PacketResult<void> SendItemUsePacket(EterBase::ItemSlot slot)
    {
        // Rzutujemy silny typ ItemSlot na strukture TItemPos
        TItemPos pos(INVENTORY, static_cast<uint16_t>(slot.value()));

        // 1. Ochrona wewnetrzna - walidacja struktury komorki ekwipunku
        if (!pos.IsValidCell())
        {
            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Warning,
                "Attempted to send ITEM_USE for invalid cell - window: {}, cell: {}",
                pos.window_type, pos.cell
            );
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        // 2. Zerowanie i uzupelnienie zserializowanego layout'u pakietu
        TPacketCGItemUse itemUsePacket{};
        itemUsePacket.header = CG::ITEM_USE;
        itemUsePacket.length = sizeof(itemUsePacket);
        itemUsePacket.pos = pos;

        // 3. Wysłanie strumienia bitowego przy użyciu API mostkowego
        if (!CPythonNetworkStream::Instance().Send(sizeof(TPacketCGItemUse), &itemUsePacket))
        {
            EterBase::ModernLogger::Log(
                EterBase::LogLevel::Error,
                "Failed to transmit ITEM_USE packet for item at window: {}, cell: {}. Session might be closed.",
                pos.window_type, pos.cell
            );
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Log(
            EterBase::LogLevel::Debug,
            "Successfully dispatched ITEM_USE packet - window: {}, cell: {}",
            pos.window_type, pos.cell
        );

        return {};
    }
}
