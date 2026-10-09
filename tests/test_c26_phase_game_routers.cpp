#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>
#include <cmath>

#include "../src/UserInterface/Contracts/IPacketRouter.h"
#include "../src/UserInterface/Contracts/IGameEvents.h"
#include "../src/UserInterface/Network/Routers/NetCombatRouter.h"
#include "../src/UserInterface/Network/Routers/NetItemRouter.h"
#include "../src/UserInterface/Network/Routers/NetActorRouter.h"
#include "../src/UserInterface/Network/Routers/NetShopRouter.h"
#include "../src/UserInterface/Network/Routers/NetExchangeRouter.h"
#include "../src/UserInterface/Network/Routers/NetQuestRouter.h"
#include "../src/UserInterface/Network/Routers/NetPartyRouter.h"
#include "../src/UserInterface/Network/Routers/NetGuildRouter.h"
#include "../src/UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include "../src/UserInterface/Core/EventBus.h"

using namespace UserInterface::Contracts;
using namespace UserInterface::Network::Routers;

// Klasa Mock dla IGameEventSink do weryfikacji publikacji zdarzen
class MockGameEventSink : public IGameEventSink
{
public:
    int itemReceivedCount = 0;
    ItemReceivedEvent lastItemEvent{};

    int attackExecutedCount = 0;
    AttackExecutedEvent lastAttackEvent{};

    int damageInfoCount = 0;
    DamageInfoEvent lastDamageEvent{};

    int actorDeadCount = 0;
    ActorDeadEvent lastDeadEvent{};

    int actorMovedCount = 0;
    ActorMovedEvent lastMovedEvent{};

    int shopCount = 0;
    ShopEvent lastShopEvent{};

    int shopSignCount = 0;
    ShopSignEvent lastShopSignEvent{};

    int exchangeCount = 0;
    ExchangeEvent lastExchangeEvent{};

    int questInfoCount = 0;
    QuestInfoEvent lastQuestInfoEvent{};

    int questConfirmCount = 0;
    QuestConfirmEvent lastQuestConfirmEvent{};

    int partyAddCount = 0;
    PartyAddEvent lastPartyAddEvent{};

    int partyUpdateCount = 0;
    PartyUpdateEvent lastPartyUpdateEvent{};

    int partyRemoveCount = 0;
    PartyRemoveEvent lastPartyRemoveEvent{};

    int guildCount = 0;
    GuildEvent lastGuildEvent{};

    void OnItemReceived(const ItemReceivedEvent& event) override
    {
        ++itemReceivedCount;
        lastItemEvent = event;
    }

    void OnAttackExecuted(const AttackExecutedEvent& event) override
    {
        ++attackExecutedCount;
        lastAttackEvent = event;
    }

    void OnDamageInfo(const DamageInfoEvent& event) override
    {
        ++damageInfoCount;
        lastDamageEvent = event;
    }

    void OnActorDead(const ActorDeadEvent& event) override
    {
        ++actorDeadCount;
        lastDeadEvent = event;
    }

    void OnActorMoved(const ActorMovedEvent& event) override
    {
        ++actorMovedCount;
        lastMovedEvent = event;
    }

    void OnShop(const ShopEvent& event) override
    {
        ++shopCount;
        lastShopEvent = event;
    }

    void OnShopSign(const ShopSignEvent& event) override
    {
        ++shopSignCount;
        lastShopSignEvent = event;
    }

    void OnExchange(const ExchangeEvent& event) override
    {
        ++exchangeCount;
        lastExchangeEvent = event;
    }

    void OnQuestInfo(const QuestInfoEvent& event) override
    {
        ++questInfoCount;
        lastQuestInfoEvent = event;
    }

    void OnQuestConfirm(const QuestConfirmEvent& event) override
    {
        ++questConfirmCount;
        lastQuestConfirmEvent = event;
    }

    void OnPartyAdd(const PartyAddEvent& event) override
    {
        ++partyAddCount;
        lastPartyAddEvent = event;
    }

    void OnPartyUpdate(const PartyUpdateEvent& event) override
    {
        ++partyUpdateCount;
        lastPartyUpdateEvent = event;
    }

    void OnPartyRemove(const PartyRemoveEvent& event) override
    {
        ++partyRemoveCount;
        lastPartyRemoveEvent = event;
    }

    void OnGuild(const GuildEvent& event) override
    {
        ++guildCount;
        lastGuildEvent = event;
    }
};

void TestNetCombatRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetCombatRouter...\n";

    MockGameEventSink sink;
    NetCombatRouter combatRouter(&sink);

    assert(combatRouter.GetRouterName() == "NetCombatRouter");
    assert(combatRouter.CanHandleHeader(static_cast<uint8_t>(0x0C))); // GC::ATTACK
    assert(combatRouter.CanHandleHeader(static_cast<uint8_t>(0x10))); // GC::DAMAGE_INFO
    assert(combatRouter.CanHandleHeader(static_cast<uint8_t>(0x13))); // GC::CREATE_FLY
    assert(combatRouter.CanHandleHeader(static_cast<uint8_t>(0x1D))); // GC::DUEL_START
    assert(combatRouter.CanHandleHeader(static_cast<uint8_t>(0x2C))); // GC::FLY_TARGETING
    assert(combatRouter.CanHandleHeader(static_cast<uint16_t>(0x0410))); // 16-bit DAMAGE_INFO
    assert(!combatRouter.CanHandleHeader(static_cast<uint8_t>(0xFF)));

    // Test HandleAttack (Zero-Desync: const ref)
    TPacketGCAttack attackPack{};
    attackPack.dwVID = 1001;
    attackPack.dwVictimVID = 2002;
    attackPack.bType = 3;
    combatRouter.HandleAttack(attackPack);

    assert(sink.attackExecutedCount == 1);
    assert(sink.lastAttackEvent.dwAttackerVID == 1001);
    assert(sink.lastAttackEvent.dwVictimVID == 2002);
    assert(sink.lastAttackEvent.byMotionType == 3);

    // Test HandleDamageInfo
    TPacketGCDamageInfo damagePack{};
    damagePack.dwVID = 2002;
    damagePack.damage = 1500;
    damagePack.flag = 0x01;
    combatRouter.HandleDamageInfo(damagePack);

    assert(sink.damageInfoCount == 1);
    assert(sink.lastDamageEvent.dwVictimVID == 2002);
    assert(sink.lastDamageEvent.dwDamage == 1500);
    assert(sink.lastDamageEvent.byDamageFlag == 0x01);

    // Test HandleFly, HandleFlyTargeting, HandleDuelStart
    TPacketGCCreateFly flyPack{};
    flyPack.bType = 1;
    flyPack.dwStartVID = 1001;
    flyPack.dwEndVID = 2002;
    combatRouter.HandleFly(flyPack);

    TPacketGCFlyTargeting targetPack{};
    targetPack.dwShooterVID = 1001;
    targetPack.dwTargetVID = 2002;
    targetPack.lX = 50000;
    targetPack.lY = 60000;
    combatRouter.HandleFlyTargeting(targetPack);

    TPacketGCDuelStart duelPack{};
    combatRouter.HandleDuelStart(duelPack);

    std::cout << "[PASS] Test NetCombatRouter zakonczony sukcesem.\n";
}

void TestNetItemRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetItemRouter...\n";

    MockGameEventSink sink;
    NetItemRouter itemRouter(&sink);

    assert(itemRouter.GetRouterName() == "NetItemRouter");
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x12))); // ITEM_SET
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x13))); // ITEM_GROUND_ADD
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x14))); // ITEM_GROUND_DEL
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x15))); // QUICKSLOT_ADD
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x16))); // QUICKSLOT_DEL
    assert(itemRouter.CanHandleHeader(static_cast<uint8_t>(0x17))); // QUICKSLOT_SWAP
    assert(itemRouter.CanHandleHeader(static_cast<uint16_t>(0x0511))); // 16-bit ITEM_SET

    // Test HandleItemSet
    TPacketGCItemSet itemSetPack{};
    itemSetPack.pos.window_type = 1;
    itemSetPack.pos.cell = 5;
    itemSetPack.vnum = 1010;
    itemSetPack.count = 20;
    itemRouter.HandleItemSet(itemSetPack);

    assert(sink.itemReceivedCount == 1);
    assert(sink.lastItemEvent.dwCell == 5);
    assert(sink.lastItemEvent.dwVnum == 1010);
    assert(sink.lastItemEvent.dwCount == 20);

    // Test HandleItemDel
    TPacketGCItemDel itemDelPack{};
    itemDelPack.pos.window_type = 1;
    itemDelPack.pos.cell = 5;
    itemRouter.HandleItemDel(itemDelPack);

    // Test Ground Add / Del
    TPacketGCItemGroundAdd groundAddPack{};
    groundAddPack.dwVID = 5001;
    groundAddPack.dwVnum = 1010;
    groundAddPack.lX = 100;
    groundAddPack.lY = 200;
    groundAddPack.lZ = 0;
    itemRouter.HandleItemGroundAdd(groundAddPack);

    TPacketGCItemGroundDel groundDelPack{};
    groundDelPack.vid = 5001;
    itemRouter.HandleItemGroundDel(groundDelPack);

    // Test QuickSlot Add / Del / Swap
    TPacketGCQuickSlotAdd quickAddPack{};
    quickAddPack.pos = 0;
    quickAddPack.slot.Type = 1;
    quickAddPack.slot.Position = 5;
    itemRouter.HandleQuickSlotAdd(quickAddPack);

    TPacketGCQuickSlotDel quickDelPack{};
    quickDelPack.pos = 0;
    itemRouter.HandleQuickSlotDel(quickDelPack);

    TPacketGCQuickSlotSwap quickSwapPack{};
    quickSwapPack.pos = 0;
    quickSwapPack.change_pos = 1;
    itemRouter.HandleQuickSlotSwap(quickSwapPack);

    std::cout << "[PASS] Test NetItemRouter zakonczony sukcesem.\n";
}

void TestNetActorRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetActorRouter...\n";

    MockGameEventSink sink;
    NetActorRouter actorRouter(&sink);

    assert(actorRouter.GetRouterName() == "NetActorRouter");
    assert(actorRouter.CanHandleHeader(static_cast<uint8_t>(0x01))); // CHARACTER_ADD
    assert(actorRouter.CanHandleHeader(static_cast<uint8_t>(0x02))); // CHARACTER_DEL
    assert(actorRouter.CanHandleHeader(static_cast<uint8_t>(0x22))); // OBSERVER_MOVE
    assert(actorRouter.CanHandleHeader(static_cast<uint8_t>(0x65))); // CHAR_ADDITIONAL_INFO
    assert(actorRouter.CanHandleHeader(static_cast<uint16_t>(0x0205))); // 16-bit CHARACTER_ADD

    // Test HandleCharacterAdd
    TPacketGCCharacterAdd charAddPack{};
    charAddPack.dwVID = 3001;
    charAddPack.wRaceNum = 0;
    charAddPack.x = 1000;
    charAddPack.y = 2000;
    charAddPack.z = 0;
    actorRouter.HandleCharacterAdd(charAddPack);

    // Test HandleCharacterAdditionalInfo
    TPacketGCCharacterAdditionalInfo addInfoPack{};
    addInfoPack.dwVID = 3001;
    addInfoPack.dwGuildID = 42;
    addInfoPack.dwLevel = 99;
    actorRouter.HandleCharacterAdditionalInfo(addInfoPack);

    // Test HandleCharacterDelete -> ActorDeadEvent
    TPacketGCCharacterDelete charDelPack{};
    charDelPack.dwVID = 3001;
    actorRouter.HandleCharacterDelete(charDelPack);

    assert(sink.actorDeadCount == 1);
    assert(sink.lastDeadEvent.dwVID == 3001);

    // Test HandleObserverMove -> ActorMovedEvent
    TPacketGCObserverMove obsMovePack{};
    obsMovePack.vid = 3001;
    obsMovePack.x = 1200;
    obsMovePack.y = 2200;
    actorRouter.HandleObserverMove(obsMovePack);

    assert(sink.actorMovedCount == 1);
    assert(sink.lastMovedEvent.dwVID == 3001);
    assert(sink.lastMovedEvent.lX == 1200);
    assert(sink.lastMovedEvent.lY == 2200);

    // Test HandleSyncPosition
    TPacketGCSyncPosition syncPack{};
    syncPack.length = 16;
    actorRouter.HandleSyncPosition(syncPack);

    std::cout << "[PASS] Test NetActorRouter zakonczony sukcesem.\n";
}

void TestNetShopRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetShopRouter...\n";

    MockGameEventSink sink;
    NetShopRouter shopRouter(&sink);

    assert(shopRouter.GetRouterName() == "NetShopRouter");
    assert(shopRouter.CanHandleHeader(static_cast<uint8_t>(0x26))); // GC::SHOP
    assert(shopRouter.CanHandleHeader(static_cast<uint8_t>(0x32))); // GC::SHOP_SIGN
    assert(shopRouter.CanHandleHeader(static_cast<uint16_t>(0x0810))); // 16-bit GC::SHOP
    assert(shopRouter.CanHandleHeader(static_cast<uint16_t>(0x0811))); // 16-bit GC::SHOP_SIGN
    assert(!shopRouter.CanHandleHeader(static_cast<uint8_t>(0xFE)));

    // Test HandleShop
    TPacketGCShop shopPack{};
    shopPack.header = 0x0810;
    shopPack.length = sizeof(TPacketGCShop);
    shopPack.subheader = ShopSub::GC::START;
    shopRouter.HandleShop(shopPack);

    assert(sink.shopCount == 1);
    assert(sink.lastShopEvent.bySubHeader == ShopSub::GC::START);

    // Test HandleShopSign
    TPacketGCShopSign signPack{};
    signPack.header = 0x0811;
    signPack.length = sizeof(TPacketGCShopSign);
    signPack.dwVID = 7777;
    std::snprintf(signPack.szSign, sizeof(signPack.szSign), "Sklep Testowy");
    shopRouter.HandleShopSign(signPack);

    assert(sink.shopSignCount == 1);
    assert(sink.lastShopSignEvent.dwVID == 7777);
    assert(sink.lastShopSignEvent.szSign == "Sklep Testowy");

    // Test HandleShopUpdatePrice
    TPacketGCShopUpdatePrice pricePack{};
    pricePack.iElkAmount = 500000;
    shopRouter.HandleShopUpdatePrice(pricePack);

    std::cout << "[PASS] Test NetShopRouter zakonczony sukcesem.\n";
}

void TestNetExchangeRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetExchangeRouter...\n";

    MockGameEventSink sink;
    NetExchangeRouter exchangeRouter(&sink);

    assert(exchangeRouter.GetRouterName() == "NetExchangeRouter");
    assert(exchangeRouter.CanHandleHeader(static_cast<uint8_t>(0x19))); // GC::EXCHANGE
    assert(exchangeRouter.CanHandleHeader(static_cast<uint16_t>(0x051C))); // 16-bit GC::EXCHANGE
    assert(!exchangeRouter.CanHandleHeader(static_cast<uint8_t>(0xFE)));

    // Test HandleExchange START
    TPacketGCExchange startPack{};
    startPack.header = 0x051C;
    startPack.length = sizeof(TPacketGCExchange);
    startPack.subheader = ExchangeSub::GC::START;
    startPack.is_me = 0;
    startPack.arg1 = 12345; // target VID
    exchangeRouter.HandleExchange(startPack);

    assert(sink.exchangeCount == 1);
    assert(sink.lastExchangeEvent.bySubHeader == ExchangeSub::GC::START);
    assert(!sink.lastExchangeEvent.bIsMe);
    assert(sink.lastExchangeEvent.dwArg1 == 12345);

    // Test HandleExchange ELK_ADD
    TPacketGCExchange elkPack{};
    elkPack.header = 0x051C;
    elkPack.length = sizeof(TPacketGCExchange);
    elkPack.subheader = ExchangeSub::GC::ELK_ADD;
    elkPack.is_me = 1;
    elkPack.arg1 = 1000000;
    exchangeRouter.HandleExchange(elkPack);

    assert(sink.exchangeCount == 2);
    assert(sink.lastExchangeEvent.bySubHeader == ExchangeSub::GC::ELK_ADD);
    assert(sink.lastExchangeEvent.bIsMe);
    assert(sink.lastExchangeEvent.dwArg1 == 1000000);

    std::cout << "[PASS] Test NetExchangeRouter zakonczony sukcesem.\n";
}

void TestNetQuestRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetQuestRouter...\n";

    MockGameEventSink sink;
    NetQuestRouter questRouter(&sink);

    assert(questRouter.GetRouterName() == "NetQuestRouter");
    assert(questRouter.CanHandleHeader(static_cast<uint8_t>(0x25))); // GC::QUEST_INFO
    assert(questRouter.CanHandleHeader(static_cast<uint16_t>(0x0911))); // 16-bit GC::QUEST_CONFIRM
    assert(questRouter.CanHandleHeader(static_cast<uint16_t>(0x0912))); // 16-bit GC::QUEST_INFO
    assert(!questRouter.CanHandleHeader(static_cast<uint8_t>(0xFE)));

    // Test HandleQuestInfo
    TPacketGCQuestInfo infoPack{};
    infoPack.header = 0x0912;
    infoPack.length = sizeof(TPacketGCQuestInfo);
    infoPack.index = 42;
    infoPack.flag = 1;
    questRouter.HandleQuestInfo(infoPack);

    assert(sink.questInfoCount == 1);
    assert(sink.lastQuestInfoEvent.wIndex == 42);
    assert(sink.lastQuestInfoEvent.byFlag == 1);

    // Test HandleQuestConfirm
    TPacketGCQuestConfirm confirmPack{};
    confirmPack.header = 0x0911;
    confirmPack.length = sizeof(TPacketGCQuestConfirm);
    std::snprintf(confirmPack.msg, sizeof(confirmPack.msg), "Czy akceptujesz misje?");
    confirmPack.timeout = 30;
    confirmPack.requestPID = 9999;
    questRouter.HandleQuestConfirm(confirmPack);

    assert(sink.questConfirmCount == 1);
    assert(sink.lastQuestConfirmEvent.szMsg == "Czy akceptujesz misje?");
    assert(sink.lastQuestConfirmEvent.lTimeout == 30);
    assert(sink.lastQuestConfirmEvent.dwRequestPID == 9999);

    std::cout << "[PASS] Test NetQuestRouter zakonczony sukcesem.\n";
}

void TestNetPartyRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetPartyRouter...\n";

    MockGameEventSink sink;
    NetPartyRouter partyRouter(&sink);

    assert(partyRouter.GetRouterName() == "NetPartyRouter");
    assert(partyRouter.CanHandleHeader(static_cast<uint8_t>(0x2E))); // GC::PARTY_INVITE
    assert(partyRouter.CanHandleHeader(static_cast<uint8_t>(0x2F))); // GC::PARTY_ADD
    assert(partyRouter.CanHandleHeader(static_cast<uint8_t>(0x30))); // GC::PARTY_UPDATE
    assert(partyRouter.CanHandleHeader(static_cast<uint8_t>(0x31))); // GC::PARTY_REMOVE
    assert(partyRouter.CanHandleHeader(static_cast<uint16_t>(0x0711))); // 16-bit GC::PARTY_ADD
    assert(!partyRouter.CanHandleHeader(static_cast<uint8_t>(0xFE)));

    // Test HandlePartyAdd
    TPacketGCPartyAdd addPack{};
    addPack.header = 0x0711;
    addPack.length = sizeof(TPacketGCPartyAdd);
    addPack.pid = 1234;
    std::snprintf(addPack.name, sizeof(addPack.name), "Towarzysz");
    partyRouter.HandlePartyAdd(addPack);

    assert(sink.partyAddCount == 1);
    assert(sink.lastPartyAddEvent.dwPID == 1234);
    assert(sink.lastPartyAddEvent.szName == "Towarzysz");

    // Test HandlePartyUpdate
    TPacketGCPartyUpdate updatePack{};
    updatePack.header = 0x0712;
    updatePack.length = sizeof(TPacketGCPartyUpdate);
    updatePack.pid = 1234;
    updatePack.state = 2;
    updatePack.percent_hp = 85;
    partyRouter.HandlePartyUpdate(updatePack);

    assert(sink.partyUpdateCount == 1);
    assert(sink.lastPartyUpdateEvent.dwPID == 1234);
    assert(sink.lastPartyUpdateEvent.byState == 2);
    assert(sink.lastPartyUpdateEvent.byPercentHP == 85);

    // Test HandlePartyRemove
    TPacketGCPartyRemove removePack{};
    removePack.header = 0x0713;
    removePack.length = sizeof(TPacketGCPartyRemove);
    removePack.pid = 1234;
    partyRouter.HandlePartyRemove(removePack);

    assert(sink.partyRemoveCount == 1);
    assert(sink.lastPartyRemoveEvent.dwPID == 1234);

    std::cout << "[PASS] Test NetPartyRouter zakonczony sukcesem.\n";
}

void TestNetGuildRouter()
{
    std::cout << "[TEST] Rozpoczecie testu NetGuildRouter...\n";

    MockGameEventSink sink;
    NetGuildRouter guildRouter(&sink);

    assert(guildRouter.GetRouterName() == "NetGuildRouter");
    assert(guildRouter.CanHandleHeader(static_cast<uint8_t>(0x33))); // GC::GUILD
    assert(guildRouter.CanHandleHeader(static_cast<uint16_t>(0x0730))); // 16-bit GC::GUILD
    assert(!guildRouter.CanHandleHeader(static_cast<uint8_t>(0xFE)));

    // Test HandleGuild
    TPacketGCGuild guildPack{};
    guildPack.header = 0x0730;
    guildPack.length = sizeof(TPacketGCGuild);
    guildPack.subheader = GuildSub::GC::INFO;
    guildRouter.HandleGuild(guildPack);

    assert(sink.guildCount == 1);
    assert(sink.lastGuildEvent.bySubHeader == GuildSub::GC::INFO);

    // Test HandleGuildWar
    TPacketGCGuildWar warPack{};
    warPack.dwGuildSelf = 100;
    warPack.dwGuildOpp = 200;
    warPack.bType = 1;
    warPack.bWarState = GUILD_WAR_ON_WAR;
    guildRouter.HandleGuildWar(warPack);

    std::cout << "[PASS] Test NetGuildRouter zakonczony sukcesem.\n";
}

void TestPhaseGamePacketDispatcher()
{
    std::cout << "[TEST] Rozpoczecie testu PhaseGamePacketDispatcher...\n";

    MockGameEventSink sink;
    PhaseGamePacketDispatcher dispatcher;

    NetCombatRouter combatRouter(&sink);
    NetItemRouter itemRouter(&sink);
    NetActorRouter actorRouter(&sink);
    NetShopRouter shopRouter(&sink);
    NetExchangeRouter exchangeRouter(&sink);
    NetQuestRouter questRouter(&sink);
    NetPartyRouter partyRouter(&sink);
    NetGuildRouter guildRouter(&sink);

    dispatcher.SetCombatRouter(&combatRouter);
    dispatcher.SetItemRouter(&itemRouter);
    dispatcher.SetActorRouter(&actorRouter);
    dispatcher.SetShopRouter(&shopRouter);
    dispatcher.SetExchangeRouter(&exchangeRouter);
    dispatcher.SetQuestRouter(&questRouter);
    dispatcher.SetPartyRouter(&partyRouter);
    dispatcher.SetGuildRouter(&guildRouter);

    assert(dispatcher.GetCombatRouter() == &combatRouter);
    assert(dispatcher.GetItemRouter() == &itemRouter);
    assert(dispatcher.GetActorRouter() == &actorRouter);
    assert(dispatcher.GetShopRouter() == &shopRouter);
    assert(dispatcher.GetExchangeRouter() == &exchangeRouter);
    assert(dispatcher.GetQuestRouter() == &questRouter);
    assert(dispatcher.GetPartyRouter() == &partyRouter);
    assert(dispatcher.GetGuildRouter() == &guildRouter);

    // Weryfikacja Jump Table O(1) dla naglowkow 8-bitowych
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x0C)) == &combatRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x12)) == &itemRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x01)) == &actorRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x26)) == &shopRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x19)) == &exchangeRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x25)) == &questRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x2F)) == &partyRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x33)) == &guildRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0xFE)) == nullptr);

    assert(dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0x26)));
    assert(dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0x19)));
    assert(!dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0xFE)));

    // Weryfikacja naglowkow 16-bitowych
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0410)) == &combatRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0511)) == &itemRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0205)) == &actorRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0810)) == &shopRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x051C)) == &exchangeRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0912)) == &questRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0711)) == &partyRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0730)) == &guildRouter);

    // Test dedykowanych metod dyspozycji sklepu
    TPacketGCShop shopPack{};
    shopPack.subheader = ShopSub::GC::START;
    bool bDispatched = dispatcher.DispatchShop(shopPack);
    assert(bDispatched);
    assert(sink.shopCount == 1);

    // Test dedykowanych metod dyspozycji wymiany
    TPacketGCExchange exchPack{};
    exchPack.subheader = ExchangeSub::GC::ACCEPT;
    exchPack.is_me = 1;
    exchPack.arg1 = 1;
    bDispatched = dispatcher.DispatchExchange(exchPack);
    assert(bDispatched);
    assert(sink.exchangeCount == 1);

    // Test dedykowanych metod dyspozycji questu
    TPacketGCQuestInfo qInfoPack{};
    qInfoPack.index = 100;
    qInfoPack.flag = 2;
    bDispatched = dispatcher.DispatchQuestInfo(qInfoPack);
    assert(bDispatched);
    assert(sink.questInfoCount == 1);

    // Test dedykowanych metod dyspozycji party
    TPacketGCPartyAdd pAddPack{};
    pAddPack.pid = 555;
    std::snprintf(pAddPack.name, sizeof(pAddPack.name), "Wojownik");
    bDispatched = dispatcher.DispatchPartyAdd(pAddPack);
    assert(bDispatched);
    assert(sink.partyAddCount == 1);

    // Test dedykowanych metod dyspozycji gildii
    TPacketGCGuild gPack{};
    gPack.subheader = GuildSub::GC::GRADE;
    bDispatched = dispatcher.DispatchGuild(gPack);
    assert(bDispatched);
    assert(sink.guildCount == 1);

    // Test uniwersalnego DispatchPacket O(1) z 16-bitowym naglowkiem
    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x0810), &shopPack);
    assert(bDispatched);

    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x051C), &exchPack);
    assert(bDispatched);

    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x0912), &qInfoPack);
    assert(bDispatched);

    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x0711), &pAddPack);
    assert(bDispatched);

    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x0730), &gPack);
    assert(bDispatched);

    std::cout << "[PASS] Test PhaseGamePacketDispatcher zakonczony sukcesem.\n";
}

void TestNetworkStreamBridgeAndFraming()
{
    std::cout << "[TEST] Rozpoczecie symulacji NetworkStream i weryfikacji framingu x64...\n";

    MockGameEventSink sink;
    auto& dispatcher = PhaseGamePacketDispatcher::Instance();
    dispatcher.ClearRouters();
    dispatcher.RegisterDefaultRouters(&sink);

    // 1. Weryfikacja obslugi naglowkow w default routers
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0401))); // ATTACK
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0410))); // DAMAGE_INFO
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0511))); // ITEM_SET
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0205))); // CHARACTER_ADD
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0810))); // SHOP
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0811))); // SHOP_SIGN
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x051C))); // EXCHANGE
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0911))); // QUEST_CONFIRM
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0912))); // QUEST_INFO
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0710))); // PARTY_INVITE
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0711))); // PARTY_ADD
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0712))); // PARTY_UPDATE
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0713))); // PARTY_REMOVE
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0730))); // GUILD

    // 2. Symulacja odbioru pakietu GC_ATTACK (0x0401)
    TPacketGCAttack attackPacket{};
    attackPacket.header = 0x0401;
    attackPacket.length = sizeof(TPacketGCAttack);
    attackPacket.dwVID = 1337;
    attackPacket.dwVictimVID = 7331;
    attackPacket.bType = 2;

    bool dispatched = dispatcher.DispatchPacket(attackPacket.header, &attackPacket);
    assert(dispatched);
    assert(sink.lastAttackEvent.dwAttackerVID == 1337);
    assert(sink.lastAttackEvent.dwVictimVID == 7331);
    assert(sink.lastAttackEvent.byMotionType == 2);

    // 3. Symulacja odbioru pakietu GC_SHOP_SIGN (0x0811)
    TPacketGCShopSign shopSignPacket{};
    shopSignPacket.header = 0x0811;
    shopSignPacket.length = sizeof(TPacketGCShopSign);
    shopSignPacket.dwVID = 4444;
    std::snprintf(shopSignPacket.szSign, sizeof(shopSignPacket.szSign), "Dobre Przedmioty");

    dispatched = dispatcher.DispatchPacket(shopSignPacket.header, &shopSignPacket);
    assert(dispatched);
    assert(sink.lastShopSignEvent.dwVID == 4444);
    assert(sink.lastShopSignEvent.szSign == "Dobre Przedmioty");

    // 4. Weryfikacja odrzucania nieznanego opkodu bez awarii (zero-crash)
    assert(!dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0xFAFA)));
    assert(!dispatcher.DispatchPacket(static_cast<uint16_t>(0xFAFA), &attackPacket));

    dispatcher.ClearRouters();
    std::cout << "[PASS] Symulacja NetworkStream i weryfikacja framingu x64 zakonczona sukcesem.\n";
}

void TestNegativeAndTruncatedPackets()
{
    std::cout << "[TEST] Rozpoczecie testow negatywnych i ucietych buforow...\n";

    MockGameEventSink sink;
    auto& dispatcher = PhaseGamePacketDispatcher::Instance();
    dispatcher.RegisterDefaultRouters(&sink);

    // 1. DispatchPacket z nullptr -> musi zwrocic false
    assert(!dispatcher.DispatchPacket(static_cast<uint16_t>(0x0401), nullptr, 0));
    assert(!dispatcher.DispatchPacket(static_cast<uint8_t>(0x0C), nullptr, 0));

    // 2. DispatchPacket z obcietym buforem (payloadSize < sizeof(TPacketGCAttack))
    TPacketGCAttack attackPacket{};
    attackPacket.header = 0x0401;
    attackPacket.length = sizeof(TPacketGCAttack);
    attackPacket.dwVID = 555;
    attackPacket.dwVictimVID = 666;

    // Za krotki bufor (np. tylko 4 bajty zamiast pelnego rozmiaru TPacketGCAttack)
    bool res = dispatcher.DispatchPacket(attackPacket.header, &attackPacket, 4);
    assert(!res); // Walidacja dlugosci odrzucila uciety pakiet

    // 3. DispatchPacket z poprawnym buforem
    res = dispatcher.DispatchPacket(attackPacket.header, &attackPacket, sizeof(TPacketGCAttack));
    assert(res);
    assert(sink.lastAttackEvent.dwAttackerVID == 555);

    // 4. DispatchPacket 8-bitowy z obcietym buforem
    res = dispatcher.DispatchPacket(static_cast<uint8_t>(0x0C), &attackPacket, 2);
    assert(!res);

    // 5. Nieznany naglowek z prawidlowym rozmiarem -> bezpieczne false bez crasha
    res = dispatcher.DispatchPacket(static_cast<uint16_t>(0xDEAD), &attackPacket, sizeof(TPacketGCAttack));
    assert(!res);

    dispatcher.ClearRouters();
    std::cout << "[PASS] Testy negatywne i walidacja ucietych buforow zakonczone sukcesem.\n";
}

int main()
{
    std::cout << "========================================================\n";
    std::cout << "URUCHOMIENIE TESTOW: PhaseGame Packet Routers & Dispatcher\n";
    std::cout << "========================================================\n";

    TestNetCombatRouter();
    TestNetItemRouter();
    TestNetActorRouter();
    TestNetShopRouter();
    TestNetExchangeRouter();
    TestNetQuestRouter();
    TestNetPartyRouter();
    TestNetGuildRouter();
    TestPhaseGamePacketDispatcher();
    TestNetworkStreamBridgeAndFraming();
    TestNegativeAndTruncatedPackets();

    std::cout << "\n>>> WSZYSTKIE TESTY ZAKONCZONE SUKCESEM (100% PASS) <<<\n";
    return 0;
}
