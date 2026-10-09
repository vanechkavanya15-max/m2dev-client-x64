#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetQuestRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetQuestRouter::NetQuestRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetQuestRouter::GetRouterName() const noexcept
    {
        return "NetQuestRouter";
    }

    bool NetQuestRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x1E: // GC::SCRIPT (legacy 30)
        case 0x25: // GC::QUEST_INFO (legacy 37)
            return true;
        default:
            return false;
        }
    }

    bool NetQuestRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0910: // GC::SCRIPT
        case 0x0911: // GC::QUEST_CONFIRM
        case 0x0912: // GC::QUEST_INFO
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetQuestRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetQuestRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetQuestRouter::HandleQuestInfo(const TPacketGCQuestInfo& packet)
    {
        EterBase::ModernLogger::Debug("NetQuestRouter: Obsluga QuestInfo [Index: {}, Flag: {}]",
            packet.index, packet.flag);

        if (m_pEventSink)
        {
            UserInterface::Contracts::QuestInfoEvent event{};
            event.wIndex = packet.index;
            event.byFlag = packet.flag;
            m_pEventSink->OnQuestInfo(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuestInfoDomainEvent(packet.index, packet.flag)
        );
    }

    void NetQuestRouter::HandleQuestConfirm(const TPacketGCQuestConfirm& packet)
    {
        EterBase::ModernLogger::Debug("NetQuestRouter: Obsluga QuestConfirm [RequestPID: {}, Timeout: {}, Msg: {}]",
            packet.requestPID, packet.timeout, packet.msg);

        if (m_pEventSink)
        {
            UserInterface::Contracts::QuestConfirmEvent event{};
            event.szMsg = packet.msg;
            event.lTimeout = packet.timeout;
            event.dwRequestPID = packet.requestPID;
            m_pEventSink->OnQuestConfirm(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuestConfirmDomainEvent(packet.msg, packet.timeout, packet.requestPID)
        );
    }
}
