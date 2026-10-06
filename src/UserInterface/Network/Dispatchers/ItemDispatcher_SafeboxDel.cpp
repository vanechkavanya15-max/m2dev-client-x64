#include "../../StdAfx.h"
#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"

namespace Network::Dispatchers
{
    struct SafeboxItemDelEvent : public UserInterface::Core::IEvent
    {
        uint8_t cell;

        explicit SafeboxItemDelEvent(uint8_t c) : cell(c) {}
    };

    EterBase::PacketResult<void> DispatchSafeboxDel(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCItemDel))
        {
            EterBase::ModernLogger::Error("DispatchSafeboxDel: Zbyt maly bufor");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCItemDel*>(buffer.data());
        UserInterface::Core::EventBus::GetInstance().Publish(SafeboxItemDelEvent(packet->pos.cell));
        EterBase::ModernLogger::Info("DispatchSafeboxDel: Safebox item deleted from cell {}", packet->pos.cell);

        return {};
    }
}
