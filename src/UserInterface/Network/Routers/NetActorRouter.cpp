#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetActorRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetActorRouter::NetActorRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetActorRouter::GetRouterName() const noexcept
    {
        return "NetActorRouter";
    }

    bool NetActorRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x01: // GC::CHARACTER_ADD
        case 0x02: // GC::CHARACTER_DEL
        case 0x03: // GC::CHARACTER_MOVE
        case 0x05: // GC::SYNC_POSITION
        case 0x22: // GC::OBSERVER_MOVE
        case 0x65: // GC::CHAR_ADDITIONAL_INFO
            return true;
        default:
            return false;
        }
    }

    bool NetActorRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0205: // GC::CHARACTER_ADD
        case 0x0206: // GC::CHARACTER_ADD2
        case 0x0207: // GC::CHAR_ADDITIONAL_INFO
        case 0x0208: // GC::CHARACTER_DEL
        case 0x0209: // GC::CHARACTER_UPDATE
        case 0x020A: // GC::CHARACTER_UPDATE2
        case 0x0302: // GC::MOVE
        case 0x0304: // GC::SYNC_POSITION
        case 0x0B20: // GC::OBSERVER_ADD
        case 0x0B21: // GC::OBSERVER_REMOVE
        case 0x0B22: // GC::OBSERVER_MOVE
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetActorRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetActorRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetActorRouter::HandleCharacterAdd(const TPacketGCCharacterAdd& packet)
    {
        EterBase::ModernLogger::Debug("NetActorRouter: Obsluga CharacterAdd [VID: {}, Race: {}, Pos: ({}, {}, {}), Angle: {}]",
            packet.dwVID, packet.wRaceNum, packet.x, packet.y, packet.z, packet.angle);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ActorAddDomainEvent(packet.dwVID, packet.angle, packet.x, packet.y, packet.z,
                packet.bType, packet.wRaceNum, packet.bMovingSpeed, packet.bAttackSpeed, packet.bStateFlag)
        );
    }

    void NetActorRouter::HandleCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet)
    {
        EterBase::ModernLogger::Debug("NetActorRouter: Obsluga CharacterAdditionalInfo [VID: {}, Name: {}, Guild: {}, Level: {}]",
            packet.dwVID, packet.name, packet.dwGuildID, packet.dwLevel);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ActorAdditionalInfoDomainEvent(packet.dwVID, packet.name, packet.bEmpire,
                packet.dwGuildID, packet.dwLevel, packet.sAlignment, packet.bPKMode, packet.dwMountVnum)
        );
    }

    void NetActorRouter::HandleCharacterDelete(const TPacketGCCharacterDelete& packet)
    {
        EterBase::ModernLogger::Debug("NetActorRouter: Obsluga CharacterDelete [VID: {}]", packet.dwVID);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ActorDeadEvent event{};
            event.dwVID = packet.dwVID;
            m_pEventSink->OnActorDead(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ActorDeleteDomainEvent(packet.dwVID)
        );
    }

    void NetActorRouter::HandleObserverMove(const TPacketGCObserverMove& packet)
    {
        EterBase::ModernLogger::Debug("NetActorRouter: Obsluga ObserverMove [VID: {}, Pos: ({}, {})]",
            packet.vid, packet.x, packet.y);

        if (m_pEventSink)
        {
            UserInterface::Contracts::ActorMovedEvent event{};
            event.dwVID = packet.vid;
            event.lX = static_cast<int32_t>(packet.x);
            event.lY = static_cast<int32_t>(packet.y);
            event.fRot = 0.0f;
            event.dwTime = 0;
            m_pEventSink->OnActorMoved(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ObserverMoveDomainEvent(packet.vid, packet.x, packet.y)
        );
    }

    void NetActorRouter::HandleSyncPosition(const TPacketGCSyncPosition& packet)
    {
        EterBase::ModernLogger::Debug("NetActorRouter: Obsluga SyncPosition [Length: {}]", packet.length);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ActorSyncPositionDomainEvent(packet.length)
        );
    }
}
