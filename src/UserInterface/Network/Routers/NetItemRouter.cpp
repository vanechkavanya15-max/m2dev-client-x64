#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetItemRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetItemRouter::NetItemRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetItemRouter::GetRouterName() const noexcept
    {
        return "NetItemRouter";
    }

    bool NetItemRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x11: // GC::ITEM_USE
        case 0x12: // GC::ITEM_UPDATE / ITEM_SET
        case 0x13: // GC::ITEM_GROUND_ADD
        case 0x14: // GC::ITEM_GROUND_DEL
        case 0x15: // GC::QUICKSLOT_ADD
        case 0x16: // GC::QUICKSLOT_DEL
        case 0x17: // GC::QUICKSLOT_SWAP
        case 0x18: // GC::ITEM_OWNERSHIP
        case 0x20: // Alternatywny ITEM_DEL
        case 0x21: // Alternatywny ITEM_SET
            return true;
        default:
            return false;
        }
    }

    bool NetItemRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0510: // GC::ITEM_DEL
        case 0x0511: // GC::ITEM_SET
        case 0x0512: // GC::ITEM_USE
        case 0x0513: // GC::ITEM_DROP
        case 0x0514: // GC::ITEM_UPDATE
        case 0x0515: // GC::ITEM_GROUND_ADD
        case 0x0516: // GC::ITEM_GROUND_DEL
        case 0x0517: // GC::ITEM_OWNERSHIP
        case 0x0518: // GC::ITEM_GET
        case 0x0519: // GC::QUICKSLOT_ADD
        case 0x051A: // GC::QUICKSLOT_DEL
        case 0x051B: // GC::QUICKSLOT_SWAP
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetItemRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetItemRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetItemRouter::HandleItemSet(const TPacketGCItemSet& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga ItemSet [Cell: {}:{}, Vnum: {}, Count: {}]",
            static_cast<uint32_t>(packet.pos.window_type), packet.pos.cell, packet.vnum, packet.count);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ItemReceivedEvent event{};
            event.dwCell = packet.pos.cell;
            event.dwVnum = packet.vnum;
            event.dwCount = packet.count;
            m_pEventSink->OnItemReceived(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ItemSetDomainEvent(packet.pos, packet.vnum, packet.count, packet.flags, packet.anti_flags, packet.highlight)
        );
    }

    void NetItemRouter::HandleItemDel(const TPacketGCItemDel& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga ItemDel [Cell: {}:{}]",
            static_cast<uint32_t>(packet.pos.window_type), packet.pos.cell);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ItemDelDomainEvent(packet.pos)
        );
    }

    void NetItemRouter::HandleItemGroundAdd(const TPacketGCItemGroundAdd& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga ItemGroundAdd [VID: {}, Vnum: {}, Pos: ({}, {}, {})]",
            packet.dwVID, packet.dwVnum, packet.lX, packet.lY, packet.lZ);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ItemGroundAddDomainEvent(packet.dwVID, packet.dwVnum, packet.lX, packet.lY, packet.lZ)
        );
    }

    void NetItemRouter::HandleItemGroundDel(const TPacketGCItemGroundDel& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga ItemGroundDel [VID: {}]", packet.vid);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ItemGroundDelDomainEvent(packet.vid)
        );
    }

    void NetItemRouter::HandleQuickSlotAdd(const TPacketGCQuickSlotAdd& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga QuickSlotAdd [Pos: {}, Type: {}, Cell: {}]",
            packet.pos, packet.slot.Type, packet.slot.Position);

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuickSlotAddDomainEvent(packet.pos, packet.slot)
        );
    }

    void NetItemRouter::HandleQuickSlotDel(const TPacketGCQuickSlotDel& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga QuickSlotDel [Pos: {}]", packet.pos);

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuickSlotDelDomainEvent(packet.pos)
        );
    }

    void NetItemRouter::HandleQuickSlotSwap(const TPacketGCQuickSlotSwap& packet)
    {
        EterBase::ModernLogger::Debug("NetItemRouter: Obsluga QuickSlotSwap [Pos: {} -> ChangePos: {}]",
            packet.pos, packet.change_pos);

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuickSlotSwapDomainEvent(packet.pos, packet.change_pos)
        );
    }
}
