#if defined(_MSC_VER) && !defined(TEST_MODE_DISABLE_STDAFX)
#include "../../StdAfx.h"
#endif

#include "PhaseGamePacketDispatcher.h"
#include "../../../EterBase/LogModern.h"
#include <algorithm>

namespace UserInterface::Network::Routers
{
    PhaseGamePacketDispatcher::PhaseGamePacketDispatcher() noexcept
    {
        m_jumpTable8.fill(nullptr);
    }

    PhaseGamePacketDispatcher& PhaseGamePacketDispatcher::Instance() noexcept
    {
        static PhaseGamePacketDispatcher s_instance;
        return s_instance;
    }

    void PhaseGamePacketDispatcher::RegisterRouter(UserInterface::Contracts::IPacketRouter* pRouter)
    {
        if (!pRouter)
            return;

        // Dodanie do listy zarejestrowanych jesli jeszcze nie istnieje
        if (std::find(m_registeredRouters.begin(), m_registeredRouters.end(), pRouter) == m_registeredRouters.end())
        {
            m_registeredRouters.push_back(pRouter);
        }

        // Wypelnienie Jump Table O(1) dla naglowkow 8-bitowych
        for (size_t i = 0; i < m_jumpTable8.size(); ++i)
        {
            const auto bHeader = static_cast<uint8_t>(i);
            if (pRouter->CanHandleHeader(bHeader))
            {
                m_jumpTable8[i] = pRouter;
            }
        }

        EterBase::ModernLogger::Debug("PhaseGamePacketDispatcher: Zarejestrowano router {}", pRouter->GetRouterName());
    }

    void PhaseGamePacketDispatcher::UnregisterRouter(UserInterface::Contracts::IPacketRouter* pRouter)
    {
        if (!pRouter)
            return;

        // Wyczyszczenie z Jump Table
        for (size_t i = 0; i < m_jumpTable8.size(); ++i)
        {
            if (m_jumpTable8[i] == pRouter)
            {
                m_jumpTable8[i] = nullptr;
            }
        }

        // Usuniecie z listy zarejestrowanych
        auto it = std::remove(m_registeredRouters.begin(), m_registeredRouters.end(), pRouter);
        m_registeredRouters.erase(it, m_registeredRouters.end());

        if (m_pCombatRouter == pRouter)
            m_pCombatRouter = nullptr;
        if (m_pItemRouter == pRouter)
            m_pItemRouter = nullptr;
        if (m_pActorRouter == pRouter)
            m_pActorRouter = nullptr;

        EterBase::ModernLogger::Debug("PhaseGamePacketDispatcher: Wyrejestrowano router {}", pRouter->GetRouterName());
    }

    void PhaseGamePacketDispatcher::ClearRouters() noexcept
    {
        m_jumpTable8.fill(nullptr);
        m_registeredRouters.clear();
        m_pCombatRouter = nullptr;
        m_pItemRouter = nullptr;
        m_pActorRouter = nullptr;
        m_defaultCombatRouter.reset();
        m_defaultItemRouter.reset();
        m_defaultActorRouter.reset();

        EterBase::ModernLogger::Debug("PhaseGamePacketDispatcher: Wyczyszczono wszystkie zarejestrowane routery");
    }

    void PhaseGamePacketDispatcher::SetCombatRouter(NetCombatRouter* pRouter) noexcept
    {
        m_pCombatRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetCombatRouter* PhaseGamePacketDispatcher::GetCombatRouter() const noexcept
    {
        return m_pCombatRouter;
    }

    void PhaseGamePacketDispatcher::SetItemRouter(NetItemRouter* pRouter) noexcept
    {
        m_pItemRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetItemRouter* PhaseGamePacketDispatcher::GetItemRouter() const noexcept
    {
        return m_pItemRouter;
    }

    void PhaseGamePacketDispatcher::SetActorRouter(NetActorRouter* pRouter) noexcept
    {
        m_pActorRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetActorRouter* PhaseGamePacketDispatcher::GetActorRouter() const noexcept
    {
        return m_pActorRouter;
    }

    void PhaseGamePacketDispatcher::RegisterDefaultRouters(UserInterface::Contracts::IGameEventSink* pEventSink)
    {
        m_defaultCombatRouter = std::make_unique<NetCombatRouter>(pEventSink);
        m_defaultItemRouter = std::make_unique<NetItemRouter>(pEventSink);
        m_defaultActorRouter = std::make_unique<NetActorRouter>(pEventSink);

        SetCombatRouter(m_defaultCombatRouter.get());
        SetItemRouter(m_defaultItemRouter.get());
        SetActorRouter(m_defaultActorRouter.get());

        EterBase::ModernLogger::Info("PhaseGamePacketDispatcher: Zainicjalizowano domyslne routery fazy gry (Combat, Item, Actor)");
    }

    UserInterface::Contracts::IPacketRouter* PhaseGamePacketDispatcher::GetRouterForHeader(uint8_t bHeader) const noexcept
    {
        return m_jumpTable8[bHeader];
    }

    UserInterface::Contracts::IPacketRouter* PhaseGamePacketDispatcher::GetRouterForHeader(uint16_t wHeader) const noexcept
    {
        if (wHeader <= 0xFF)
        {
            return m_jumpTable8[static_cast<uint8_t>(wHeader)];
        }

        // Jump table / switch naglowkow 16-bitowych O(1)
        switch (wHeader)
        {
        case 0x0401: // ATTACK
        case 0x0406:
        case 0x0410: // GC::DAMAGE_INFO
        case 0x0411: // GC::FLY_TARGETING
        case 0x0412: // GC::ADD_FLY_TARGETING
        case 0x0413: // GC::CREATE_FLY
        case 0x0414: // GC::PVP
        case 0x0415: // GC::DUEL_START
            return m_pCombatRouter;

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
            return m_pItemRouter;

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
            return m_pActorRouter;

        default:
            return nullptr;
        }
    }

    bool PhaseGamePacketDispatcher::HasHandlerForHeader(uint8_t bHeader) const noexcept
    {
        return m_jumpTable8[bHeader] != nullptr;
    }

    bool PhaseGamePacketDispatcher::HasHandlerForHeader(uint16_t wHeader) const noexcept
    {
        return GetRouterForHeader(wHeader) != nullptr;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow walki (Combat)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchAttack(const TPacketGCAttack& packet)
    {
        if (!m_pCombatRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego CombatRouter dla Attack");
            return false;
        }
        m_pCombatRouter->HandleAttack(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchDamageInfo(const TPacketGCDamageInfo& packet)
    {
        if (!m_pCombatRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego CombatRouter dla DamageInfo");
            return false;
        }
        m_pCombatRouter->HandleDamageInfo(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchFly(const TPacketGCCreateFly& packet)
    {
        if (!m_pCombatRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego CombatRouter dla Fly");
            return false;
        }
        m_pCombatRouter->HandleFly(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchFlyTargeting(const TPacketGCFlyTargeting& packet)
    {
        if (!m_pCombatRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego CombatRouter dla FlyTargeting");
            return false;
        }
        m_pCombatRouter->HandleFlyTargeting(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchDuelStart(const TPacketGCDuelStart& packet)
    {
        if (!m_pCombatRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego CombatRouter dla DuelStart");
            return false;
        }
        m_pCombatRouter->HandleDuelStart(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow przedmiotow (Item)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchItemSet(const TPacketGCItemSet& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla ItemSet");
            return false;
        }
        m_pItemRouter->HandleItemSet(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchItemDel(const TPacketGCItemDel& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla ItemDel");
            return false;
        }
        m_pItemRouter->HandleItemDel(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchItemGroundAdd(const TPacketGCItemGroundAdd& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla ItemGroundAdd");
            return false;
        }
        m_pItemRouter->HandleItemGroundAdd(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchItemGroundDel(const TPacketGCItemGroundDel& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla ItemGroundDel");
            return false;
        }
        m_pItemRouter->HandleItemGroundDel(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchQuickSlotAdd(const TPacketGCQuickSlotAdd& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla QuickSlotAdd");
            return false;
        }
        m_pItemRouter->HandleQuickSlotAdd(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchQuickSlotDel(const TPacketGCQuickSlotDel& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla QuickSlotDel");
            return false;
        }
        m_pItemRouter->HandleQuickSlotDel(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchQuickSlotSwap(const TPacketGCQuickSlotSwap& packet)
    {
        if (!m_pItemRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ItemRouter dla QuickSlotSwap");
            return false;
        }
        m_pItemRouter->HandleQuickSlotSwap(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow aktorow (Actor)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchCharacterAdd(const TPacketGCCharacterAdd& packet)
    {
        if (!m_pActorRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ActorRouter dla CharacterAdd");
            return false;
        }
        m_pActorRouter->HandleCharacterAdd(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet)
    {
        if (!m_pActorRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ActorRouter dla CharacterAdditionalInfo");
            return false;
        }
        m_pActorRouter->HandleCharacterAdditionalInfo(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchCharacterDelete(const TPacketGCCharacterDelete& packet)
    {
        if (!m_pActorRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ActorRouter dla CharacterDelete");
            return false;
        }
        m_pActorRouter->HandleCharacterDelete(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchObserverMove(const TPacketGCObserverMove& packet)
    {
        if (!m_pActorRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ActorRouter dla ObserverMove");
            return false;
        }
        m_pActorRouter->HandleObserverMove(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchSyncPosition(const TPacketGCSyncPosition& packet)
    {
        if (!m_pActorRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ActorRouter dla SyncPosition");
            return false;
        }
        m_pActorRouter->HandleSyncPosition(packet);
        return true;
    }

    // ====================================================================
    // Uniwersalna dyspozycja zdeserializowanego rekordu na bazie naglowka (O(1))
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchPacket(uint8_t bHeader, const void* pData)
    {
        if (!pData)
            return false;

        switch (bHeader)
        {
        case 0x0C: // GC::ATTACK
        case 0x1C:
            return DispatchAttack(*reinterpret_cast<const TPacketGCAttack*>(pData));
        case 0x10: // GC::DAMAGE_INFO
            return DispatchDamageInfo(*reinterpret_cast<const TPacketGCDamageInfo*>(pData));
        case 0x13: // GC::CREATE_FLY
            return DispatchFly(*reinterpret_cast<const TPacketGCCreateFly*>(pData));
        case 0x2C: // GC::FLY_TARGETING
        case 0x2D: // GC::ADD_FLY_TARGETING
            return DispatchFlyTargeting(*reinterpret_cast<const TPacketGCFlyTargeting*>(pData));
        case 0x1D: // GC::DUEL_START
            return DispatchDuelStart(*reinterpret_cast<const TPacketGCDuelStart*>(pData));

        case 0x12: // GC::ITEM_SET
        case 0x21:
            return DispatchItemSet(*reinterpret_cast<const TPacketGCItemSet*>(pData));
        case 0x20: // GC::ITEM_DEL
            return DispatchItemDel(*reinterpret_cast<const TPacketGCItemDel*>(pData));
        case 0x14: // GC::ITEM_GROUND_DEL
            return DispatchItemGroundDel(*reinterpret_cast<const TPacketGCItemGroundDel*>(pData));
        case 0x15: // GC::QUICKSLOT_ADD
            return DispatchQuickSlotAdd(*reinterpret_cast<const TPacketGCQuickSlotAdd*>(pData));
        case 0x16: // GC::QUICKSLOT_DEL
            return DispatchQuickSlotDel(*reinterpret_cast<const TPacketGCQuickSlotDel*>(pData));
        case 0x17: // GC::QUICKSLOT_SWAP
            return DispatchQuickSlotSwap(*reinterpret_cast<const TPacketGCQuickSlotSwap*>(pData));

        case 0x01: // GC::CHARACTER_ADD
            return DispatchCharacterAdd(*reinterpret_cast<const TPacketGCCharacterAdd*>(pData));
        case 0x65: // GC::CHAR_ADDITIONAL_INFO
            return DispatchCharacterAdditionalInfo(*reinterpret_cast<const TPacketGCCharacterAdditionalInfo*>(pData));
        case 0x02: // GC::CHARACTER_DEL
            return DispatchCharacterDelete(*reinterpret_cast<const TPacketGCCharacterDelete*>(pData));
        case 0x22: // GC::OBSERVER_MOVE
            return DispatchObserverMove(*reinterpret_cast<const TPacketGCObserverMove*>(pData));
        case 0x05: // GC::SYNC_POSITION
            return DispatchSyncPosition(*reinterpret_cast<const TPacketGCSyncPosition*>(pData));

        default:
            EterBase::ModernLogger::Trace("PhaseGamePacketDispatcher: Nieobslugiwany naglowek 8-bitowy 0x{:02X}", bHeader);
            return false;
        }
    }

    bool PhaseGamePacketDispatcher::DispatchPacket(uint16_t wHeader, const void* pData)
    {
        if (!pData)
            return false;

        switch (wHeader)
        {
        case 0x0401: // ATTACK
        case 0x0406:
            return DispatchAttack(*reinterpret_cast<const TPacketGCAttack*>(pData));
        case 0x0410: // GC::DAMAGE_INFO
            return DispatchDamageInfo(*reinterpret_cast<const TPacketGCDamageInfo*>(pData));
        case 0x0411: // GC::FLY_TARGETING
        case 0x0412: // GC::ADD_FLY_TARGETING
            return DispatchFlyTargeting(*reinterpret_cast<const TPacketGCFlyTargeting*>(pData));
        case 0x0413: // GC::CREATE_FLY
            return DispatchFly(*reinterpret_cast<const TPacketGCCreateFly*>(pData));
        case 0x0415: // GC::DUEL_START
            return DispatchDuelStart(*reinterpret_cast<const TPacketGCDuelStart*>(pData));

        case 0x0510: // GC::ITEM_DEL
            return DispatchItemDel(*reinterpret_cast<const TPacketGCItemDel*>(pData));
        case 0x0511: // GC::ITEM_SET
            return DispatchItemSet(*reinterpret_cast<const TPacketGCItemSet*>(pData));
        case 0x0515: // GC::ITEM_GROUND_ADD
            return DispatchItemGroundAdd(*reinterpret_cast<const TPacketGCItemGroundAdd*>(pData));
        case 0x0516: // GC::ITEM_GROUND_DEL
            return DispatchItemGroundDel(*reinterpret_cast<const TPacketGCItemGroundDel*>(pData));
        case 0x0519: // GC::QUICKSLOT_ADD
            return DispatchQuickSlotAdd(*reinterpret_cast<const TPacketGCQuickSlotAdd*>(pData));
        case 0x051A: // GC::QUICKSLOT_DEL
            return DispatchQuickSlotDel(*reinterpret_cast<const TPacketGCQuickSlotDel*>(pData));
        case 0x051B: // GC::QUICKSLOT_SWAP
            return DispatchQuickSlotSwap(*reinterpret_cast<const TPacketGCQuickSlotSwap*>(pData));

        case 0x0205: // GC::CHARACTER_ADD
            return DispatchCharacterAdd(*reinterpret_cast<const TPacketGCCharacterAdd*>(pData));
        case 0x0207: // GC::CHAR_ADDITIONAL_INFO
            return DispatchCharacterAdditionalInfo(*reinterpret_cast<const TPacketGCCharacterAdditionalInfo*>(pData));
        case 0x0208: // GC::CHARACTER_DEL
            return DispatchCharacterDelete(*reinterpret_cast<const TPacketGCCharacterDelete*>(pData));
        case 0x0304: // GC::SYNC_POSITION
            return DispatchSyncPosition(*reinterpret_cast<const TPacketGCSyncPosition*>(pData));
        case 0x0B22: // GC::OBSERVER_MOVE
            return DispatchObserverMove(*reinterpret_cast<const TPacketGCObserverMove*>(pData));

        default:
            if (wHeader <= 0xFF)
            {
                return DispatchPacket(static_cast<uint8_t>(wHeader), pData);
            }
            EterBase::ModernLogger::Trace("PhaseGamePacketDispatcher: Nieobslugiwany naglowek 16-bitowy 0x{:04X}", wHeader);
            return false;
        }
    }
}
