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
    struct SafeboxItemSetEvent : public UserInterface::Core::IEvent
    {
        uint8_t cell;
        uint32_t vnum;
        uint8_t count;

        SafeboxItemSetEvent(uint8_t c, uint32_t v, uint8_t cnt) : cell(c), vnum(v), count(cnt) {}
    };

    EterBase::PacketResult<void> DispatchSafeboxSet(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCItemSet))
        {
            EterBase::ModernLogger::Error("DispatchSafeboxSet: Zbyt maly bufor ({} < {})", buffer.size(), sizeof(TPacketGCItemSet));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCItemSet*>(buffer.data());
        UserInterface::Core::EventBus::GetInstance().Publish(SafeboxItemSetEvent(packet->pos.cell, packet->vnum, packet->count));
        EterBase::ModernLogger::Info("DispatchSafeboxSet: Safebox item set cell {} vnum {} count {}", packet->pos.cell, packet->vnum, packet->count);

        return {};
    }
}
