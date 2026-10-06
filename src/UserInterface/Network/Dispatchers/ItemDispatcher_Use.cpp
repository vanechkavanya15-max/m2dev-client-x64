#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <span>
#include <cstring>

namespace Network::Dispatchers
{
    // Define the event locally strictly adhering to zero-conflict
    struct ItemUseDispatchedEvent : public UserInterface::Core::IEvent
    {
        EterBase::ItemSlot cell;
        EterBase::EntityId ch_vid;
        EterBase::EntityId victim_vid;
        EterBase::ItemVnum vnum;

        ItemUseDispatchedEvent(EterBase::ItemSlot cell, EterBase::EntityId ch_vid, EterBase::EntityId victim_vid, EterBase::ItemVnum vnum)
            : cell(cell), ch_vid(ch_vid), victim_vid(victim_vid), vnum(vnum) {}
    };

    /**
     * @brief Zaimplementuj kompletny plik src/UserInterface/Network/Dispatchers/ItemDispatcher_Use.cpp.
     * Przetwarza pakiet uzycia przedmiotu (TPacketGCItemUse) odebrany z serwera, uzywajac std::span do bezpiecznego parsera.
     * Uzywamy EterBase::PacketResult<void> do zglaszania statusow bledu zamiast wylatywania / crashowania.
     */
    EterBase::PacketResult<void> ProcessItemUse(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCItemUse))
        {
            EterBase::ModernLogger::Error("ItemDispatcher_Use: Buffer underflow. Expected >= {} bytes, got {}",
                                          sizeof(TPacketGCItemUse), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCItemUse payload{};
        std::memcpy(&payload, buffer.data(), sizeof(TPacketGCItemUse));

        const EterBase::ItemSlot cell(payload.Cell.cell);
        const EterBase::EntityId chVid(payload.ch_vid);
        const EterBase::EntityId victimVid(payload.victim_vid);
        const EterBase::ItemVnum vnum(payload.vnum);

        EterBase::ModernLogger::Info("ItemDispatcher_Use: Received ITEM_USE from GC. Cell: {}, vnum: {}",
                                     cell.value(), vnum.value());

        // Opublikuj wydarzenie uzywajac lokalnie stworzonego ItemUseDispatchedEvent aby uniknac modyfikacji innych plikow
        UserInterface::Core::EventBus::GetInstance().Publish(ItemUseDispatchedEvent(cell, chVid, victimVid, vnum));

        return {};
    }
}
