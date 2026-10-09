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
        if (m_pShopRouter == pRouter)
            m_pShopRouter = nullptr;
        if (m_pExchangeRouter == pRouter)
            m_pExchangeRouter = nullptr;
        if (m_pQuestRouter == pRouter)
            m_pQuestRouter = nullptr;
        if (m_pPartyRouter == pRouter)
            m_pPartyRouter = nullptr;
        if (m_pGuildRouter == pRouter)
            m_pGuildRouter = nullptr;

        EterBase::ModernLogger::Debug("PhaseGamePacketDispatcher: Wyrejestrowano router {}", pRouter->GetRouterName());
    }

    void PhaseGamePacketDispatcher::ClearRouters() noexcept
    {
        m_jumpTable8.fill(nullptr);
        m_registeredRouters.clear();
        m_pCombatRouter = nullptr;
        m_pItemRouter = nullptr;
        m_pActorRouter = nullptr;
        m_pShopRouter = nullptr;
        m_pExchangeRouter = nullptr;
        m_pQuestRouter = nullptr;
        m_pPartyRouter = nullptr;
        m_pGuildRouter = nullptr;
        m_defaultCombatRouter.reset();
        m_defaultItemRouter.reset();
        m_defaultActorRouter.reset();
        m_defaultShopRouter.reset();
        m_defaultExchangeRouter.reset();
        m_defaultQuestRouter.reset();
        m_defaultPartyRouter.reset();
        m_defaultGuildRouter.reset();

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

    void PhaseGamePacketDispatcher::SetShopRouter(NetShopRouter* pRouter) noexcept
    {
        m_pShopRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetShopRouter* PhaseGamePacketDispatcher::GetShopRouter() const noexcept
    {
        return m_pShopRouter;
    }

    void PhaseGamePacketDispatcher::SetExchangeRouter(NetExchangeRouter* pRouter) noexcept
    {
        m_pExchangeRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetExchangeRouter* PhaseGamePacketDispatcher::GetExchangeRouter() const noexcept
    {
        return m_pExchangeRouter;
    }

    void PhaseGamePacketDispatcher::SetQuestRouter(NetQuestRouter* pRouter) noexcept
    {
        m_pQuestRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetQuestRouter* PhaseGamePacketDispatcher::GetQuestRouter() const noexcept
    {
        return m_pQuestRouter;
    }

    void PhaseGamePacketDispatcher::SetPartyRouter(NetPartyRouter* pRouter) noexcept
    {
        m_pPartyRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetPartyRouter* PhaseGamePacketDispatcher::GetPartyRouter() const noexcept
    {
        return m_pPartyRouter;
    }

    void PhaseGamePacketDispatcher::SetGuildRouter(NetGuildRouter* pRouter) noexcept
    {
        m_pGuildRouter = pRouter;
        if (pRouter)
        {
            RegisterRouter(pRouter);
        }
    }

    NetGuildRouter* PhaseGamePacketDispatcher::GetGuildRouter() const noexcept
    {
        return m_pGuildRouter;
    }

    void PhaseGamePacketDispatcher::RegisterDefaultRouters(UserInterface::Contracts::IGameEventSink* pEventSink)
    {
        m_defaultCombatRouter = std::make_unique<NetCombatRouter>(pEventSink);
        m_defaultItemRouter = std::make_unique<NetItemRouter>(pEventSink);
        m_defaultActorRouter = std::make_unique<NetActorRouter>(pEventSink);
        m_defaultShopRouter = std::make_unique<NetShopRouter>(pEventSink);
        m_defaultExchangeRouter = std::make_unique<NetExchangeRouter>(pEventSink);
        m_defaultQuestRouter = std::make_unique<NetQuestRouter>(pEventSink);
        m_defaultPartyRouter = std::make_unique<NetPartyRouter>(pEventSink);
        m_defaultGuildRouter = std::make_unique<NetGuildRouter>(pEventSink);

        SetCombatRouter(m_defaultCombatRouter.get());
        SetItemRouter(m_defaultItemRouter.get());
        SetActorRouter(m_defaultActorRouter.get());
        SetShopRouter(m_defaultShopRouter.get());
        SetExchangeRouter(m_defaultExchangeRouter.get());
        SetQuestRouter(m_defaultQuestRouter.get());
        SetPartyRouter(m_defaultPartyRouter.get());
        SetGuildRouter(m_defaultGuildRouter.get());

        EterBase::ModernLogger::Info("PhaseGamePacketDispatcher: Zainicjalizowano domyslne routery fazy gry (Combat, Item, Actor, Shop, Exchange, Quest, Party, Guild)");
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

        case 0x0802: // GC::MYSHOP
        case 0x0810: // GC::SHOP
        case 0x0811: // GC::SHOP_SIGN
            return m_pShopRouter;

        case 0x051C: // GC::EXCHANGE
            return m_pExchangeRouter;

        case 0x0910: // GC::SCRIPT
        case 0x0911: // GC::QUEST_CONFIRM
        case 0x0912: // GC::QUEST_INFO
            return m_pQuestRouter;

        case 0x0710: // GC::PARTY_INVITE
        case 0x0711: // GC::PARTY_ADD
        case 0x0712: // GC::PARTY_UPDATE
        case 0x0713: // GC::PARTY_REMOVE
        case 0x0714: // GC::PARTY_LINK
        case 0x0715: // GC::PARTY_UNLINK
        case 0x0716: // GC::PARTY_PARAMETER
            return m_pPartyRouter;

        case 0x0730: // GC::GUILD
        case 0x0731: // GC::REQUEST_MAKE_GUILD
        case 0x0732: // GC::SYMBOL_DATA
            return m_pGuildRouter;

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
    // Dyspozycja zdeserializowanych pakietow sklepu (Shop)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchShop(const TPacketGCShop& packet)
    {
        if (!m_pShopRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ShopRouter dla Shop");
            return false;
        }
        m_pShopRouter->HandleShop(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchShopSign(const TPacketGCShopSign& packet)
    {
        if (!m_pShopRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ShopRouter dla ShopSign");
            return false;
        }
        m_pShopRouter->HandleShopSign(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchShopStart(const TPacketGCShopStart& packet)
    {
        if (!m_pShopRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ShopRouter dla ShopStart");
            return false;
        }
        m_pShopRouter->HandleShopStart(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchShopUpdateItem(const TPacketGCShopUpdateItem& packet)
    {
        if (!m_pShopRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ShopRouter dla ShopUpdateItem");
            return false;
        }
        m_pShopRouter->HandleShopUpdateItem(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchShopUpdatePrice(const TPacketGCShopUpdatePrice& packet)
    {
        if (!m_pShopRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ShopRouter dla ShopUpdatePrice");
            return false;
        }
        m_pShopRouter->HandleShopUpdatePrice(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow handlu p2p (Exchange)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchExchange(const TPacketGCExchange& packet)
    {
        if (!m_pExchangeRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego ExchangeRouter dla Exchange");
            return false;
        }
        m_pExchangeRouter->HandleExchange(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow zadan (Quest)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchQuestInfo(const TPacketGCQuestInfo& packet)
    {
        if (!m_pQuestRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego QuestRouter dla QuestInfo");
            return false;
        }
        m_pQuestRouter->HandleQuestInfo(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchQuestConfirm(const TPacketGCQuestConfirm& packet)
    {
        if (!m_pQuestRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego QuestRouter dla QuestConfirm");
            return false;
        }
        m_pQuestRouter->HandleQuestConfirm(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow druzyny (Party)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchPartyInvite(const TPacketGCPartyInvite& packet)
    {
        if (!m_pPartyRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego PartyRouter dla PartyInvite");
            return false;
        }
        m_pPartyRouter->HandlePartyInvite(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchPartyAdd(const TPacketGCPartyAdd& packet)
    {
        if (!m_pPartyRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego PartyRouter dla PartyAdd");
            return false;
        }
        m_pPartyRouter->HandlePartyAdd(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchPartyUpdate(const TPacketGCPartyUpdate& packet)
    {
        if (!m_pPartyRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego PartyRouter dla PartyUpdate");
            return false;
        }
        m_pPartyRouter->HandlePartyUpdate(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchPartyRemove(const TPacketGCPartyRemove& packet)
    {
        if (!m_pPartyRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego PartyRouter dla PartyRemove");
            return false;
        }
        m_pPartyRouter->HandlePartyRemove(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchPartyParameter(const TPacketGCPartyParameter& packet)
    {
        if (!m_pPartyRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego PartyRouter dla PartyParameter");
            return false;
        }
        m_pPartyRouter->HandlePartyParameter(packet);
        return true;
    }

    // ====================================================================
    // Dyspozycja zdeserializowanych pakietow gildii (Guild)
    // ====================================================================

    bool PhaseGamePacketDispatcher::DispatchGuild(const TPacketGCGuild& packet)
    {
        if (!m_pGuildRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego GuildRouter dla Guild");
            return false;
        }
        m_pGuildRouter->HandleGuild(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchGuildWar(const TPacketGCGuildWar& packet)
    {
        if (!m_pGuildRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego GuildRouter dla GuildWar");
            return false;
        }
        m_pGuildRouter->HandleGuildWar(packet);
        return true;
    }

    bool PhaseGamePacketDispatcher::DispatchGuildWarPoint(const TPacketGuildWarPoint& packet)
    {
        if (!m_pGuildRouter)
        {
            EterBase::ModernLogger::Warn("PhaseGamePacketDispatcher: Brak zarejestrowanego GuildRouter dla GuildWarPoint");
            return false;
        }
        m_pGuildRouter->HandleGuildWarPoint(packet);
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

        case 0x26: // GC::SHOP
            return DispatchShop(*reinterpret_cast<const TPacketGCShop*>(pData));
        case 0x32: // GC::SHOP_SIGN
            return DispatchShopSign(*reinterpret_cast<const TPacketGCShopSign*>(pData));

        case 0x19: // GC::EXCHANGE
            return DispatchExchange(*reinterpret_cast<const TPacketGCExchange*>(pData));

        case 0x25: // GC::QUEST_INFO
            return DispatchQuestInfo(*reinterpret_cast<const TPacketGCQuestInfo*>(pData));

        case 0x2E: // GC::PARTY_INVITE
            return DispatchPartyInvite(*reinterpret_cast<const TPacketGCPartyInvite*>(pData));
        case 0x2F: // GC::PARTY_ADD
            return DispatchPartyAdd(*reinterpret_cast<const TPacketGCPartyAdd*>(pData));
        case 0x30: // GC::PARTY_UPDATE
            return DispatchPartyUpdate(*reinterpret_cast<const TPacketGCPartyUpdate*>(pData));
        case 0x31: // GC::PARTY_REMOVE
            return DispatchPartyRemove(*reinterpret_cast<const TPacketGCPartyRemove*>(pData));

        case 0x33: // GC::GUILD
            return DispatchGuild(*reinterpret_cast<const TPacketGCGuild*>(pData));

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

        case 0x0810: // GC::SHOP
            return DispatchShop(*reinterpret_cast<const TPacketGCShop*>(pData));
        case 0x0811: // GC::SHOP_SIGN
            return DispatchShopSign(*reinterpret_cast<const TPacketGCShopSign*>(pData));

        case 0x051C: // GC::EXCHANGE
            return DispatchExchange(*reinterpret_cast<const TPacketGCExchange*>(pData));

        case 0x0911: // GC::QUEST_CONFIRM
            return DispatchQuestConfirm(*reinterpret_cast<const TPacketGCQuestConfirm*>(pData));
        case 0x0912: // GC::QUEST_INFO
            return DispatchQuestInfo(*reinterpret_cast<const TPacketGCQuestInfo*>(pData));

        case 0x0710: // GC::PARTY_INVITE
            return DispatchPartyInvite(*reinterpret_cast<const TPacketGCPartyInvite*>(pData));
        case 0x0711: // GC::PARTY_ADD
            return DispatchPartyAdd(*reinterpret_cast<const TPacketGCPartyAdd*>(pData));
        case 0x0712: // GC::PARTY_UPDATE
            return DispatchPartyUpdate(*reinterpret_cast<const TPacketGCPartyUpdate*>(pData));
        case 0x0713: // GC::PARTY_REMOVE
            return DispatchPartyRemove(*reinterpret_cast<const TPacketGCPartyRemove*>(pData));
        case 0x0716: // GC::PARTY_PARAMETER
            return DispatchPartyParameter(*reinterpret_cast<const TPacketGCPartyParameter*>(pData));

        case 0x0730: // GC::GUILD
            return DispatchGuild(*reinterpret_cast<const TPacketGCGuild*>(pData));

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
