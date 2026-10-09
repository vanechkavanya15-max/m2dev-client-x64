#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "NetCombatRouter.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Network::Routers
{
    NetCombatRouter::NetCombatRouter(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept
        : m_pEventSink(pEventSink)
    {
    }

    std::string_view NetCombatRouter::GetRouterName() const noexcept
    {
        return "NetCombatRouter";
    }

    bool NetCombatRouter::CanHandleHeader(uint8_t bHeader) const noexcept
    {
        switch (bHeader)
        {
        case 0x0C: // Beavium GC::ATTACK
        case 0x1C: // Klasyczny HEADER_GC_ATTACK (28)
        case 0x10: // GC::DAMAGE_INFO (16)
        case 0x13: // GC::CREATE_FLY (19)
        case 0x1D: // GC::DUEL_START (29)
        case 0x2C: // GC::FLY_TARGETING (44)
        case 0x2D: // GC::ADD_FLY_TARGETING (45)
            return true;
        default:
            return false;
        }
    }

    bool NetCombatRouter::CanHandleHeader(uint16_t wHeader) const noexcept
    {
        switch (wHeader)
        {
        case 0x0401: // CG/GC ATTACK opcode
        case 0x0406: // Alternatywny GC ATTACK
        case 0x0410: // GC::DAMAGE_INFO
        case 0x0411: // GC::FLY_TARGETING
        case 0x0412: // GC::ADD_FLY_TARGETING
        case 0x0413: // GC::CREATE_FLY
        case 0x0414: // GC::PVP
        case 0x0415: // GC::DUEL_START
            return true;
        default:
            if (wHeader <= 0xFF)
            {
                return CanHandleHeader(static_cast<uint8_t>(wHeader));
            }
            return false;
        }
    }

    void NetCombatRouter::SetEventSink(UserInterface::Contracts::IGameEventSink* pSink) noexcept
    {
        m_pEventSink = pSink;
    }

    UserInterface::Contracts::IGameEventSink* NetCombatRouter::GetEventSink() const noexcept
    {
        return m_pEventSink;
    }

    void NetCombatRouter::HandleAttack(const TPacketGCAttack& packet)
    {
        EterBase::ModernLogger::Debug("NetCombatRouter: Obsluga ataku [AttackerVID: {}, VictimVID: {}, Type: {}]",
            packet.dwVID, packet.dwVictimVID, packet.bType);

        // Notyfikacja dedykowanego zlewu zdarzen kontraktowych
        if (m_pEventSink)
        {
            UserInterface::Contracts::AttackExecutedEvent event{};
            event.dwAttackerVID = packet.dwVID;
            event.dwVictimVID = packet.dwVictimVID;
            event.byMotionType = packet.bType;
            m_pEventSink->OnAttackExecuted(event);
        }

        // Publikacja w szynie EventBus dla reszty podsystemow
        UserInterface::Core::EventBus::GetInstance().Publish(
            CombatAttackDomainEvent(packet.dwVID, packet.dwVictimVID, packet.bType)
        );
    }

    void NetCombatRouter::HandleDamageInfo(const TPacketGCDamageInfo& packet)
    {
        EterBase::ModernLogger::Debug("NetCombatRouter: Obsluga DamageInfo [VictimVID: {}, Damage: {}, Flag: 0x{:02X}]",
            packet.dwVID, packet.damage, packet.flag);

        const uint32_t uDamage = packet.damage >= 0 ? static_cast<uint32_t>(packet.damage) : 0;

        if (m_pEventSink)
        {
            UserInterface::Contracts::DamageInfoEvent event{};
            event.dwVictimVID = packet.dwVID;
            event.dwAttackerVID = 0; // Nieznany bezposrednio z pakietu informacji o obrazeniach
            event.dwDamage = uDamage;
            event.byDamageFlag = packet.flag;
            m_pEventSink->OnDamageInfo(event);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            CombatDamageInfoDomainEvent(packet.dwVID, 0, packet.damage, packet.flag)
        );
    }

    void NetCombatRouter::HandleFly(const TPacketGCCreateFly& packet)
    {
        EterBase::ModernLogger::Debug("NetCombatRouter: Obsluga efektu lotu pocisku [Type: {}, StartVID: {}, EndVID: {}]",
            packet.bType, packet.dwStartVID, packet.dwEndVID);

        UserInterface::Core::EventBus::GetInstance().Publish(
            CombatFlyDomainEvent(packet.bType, packet.dwStartVID, packet.dwEndVID)
        );
    }

    void NetCombatRouter::HandleFlyTargeting(const TPacketGCFlyTargeting& packet)
    {
        EterBase::ModernLogger::Debug("NetCombatRouter: Obsluga celowania pocisku [ShooterVID: {}, TargetVID: {}, X: {}, Y: {}]",
            packet.dwShooterVID, packet.dwTargetVID, packet.lX, packet.lY);

        UserInterface::Core::EventBus::GetInstance().Publish(
            CombatFlyTargetingDomainEvent(packet.dwShooterVID, packet.dwTargetVID, packet.lX, packet.lY)
        );
    }

    void NetCombatRouter::HandleDuelStart(const TPacketGCDuelStart& packet)
    {
        EterBase::ModernLogger::Debug("NetCombatRouter: Obsluga rozpoczecia pojedynku (DuelStart)");

        UserInterface::Core::EventBus::GetInstance().Publish(
            CombatDuelStartDomainEvent()
        );
    }
}
