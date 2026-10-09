#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include "../src/UserInterface/Contracts/IPacketRouter.h"
#include "../src/UserInterface/Contracts/IGameEvents.h"
#include "../src/UserInterface/Network/Routers/NetCombatRouter.h"
#include "../src/UserInterface/Network/Routers/NetItemRouter.h"
#include "../src/UserInterface/Network/Routers/NetActorRouter.h"
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

void TestPhaseGamePacketDispatcher()
{
    std::cout << "[TEST] Rozpoczecie testu PhaseGamePacketDispatcher...\n";

    MockGameEventSink sink;
    PhaseGamePacketDispatcher dispatcher;

    NetCombatRouter combatRouter(&sink);
    NetItemRouter itemRouter(&sink);
    NetActorRouter actorRouter(&sink);

    dispatcher.SetCombatRouter(&combatRouter);
    dispatcher.SetItemRouter(&itemRouter);
    dispatcher.SetActorRouter(&actorRouter);

    // Weryfikacja Jump Table O(1)
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x0C)) == &combatRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x12)) == &itemRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0x01)) == &actorRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint8_t>(0xFE)) == nullptr);

    assert(dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0x0C)));
    assert(dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0x12)));
    assert(dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0x01)));
    assert(!dispatcher.HasHandlerForHeader(static_cast<uint8_t>(0xFE)));

    // Weryfikacja naglowkow 16-bitowych
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0410)) == &combatRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0511)) == &itemRouter);
    assert(dispatcher.GetRouterForHeader(static_cast<uint16_t>(0x0205)) == &actorRouter);

    // Test dedykowanych metod dyspozycji
    TPacketGCAttack attack{};
    attack.dwVID = 50;
    attack.dwVictimVID = 60;
    attack.bType = 1;
    bool bDispatched = dispatcher.DispatchAttack(attack);
    assert(bDispatched);
    assert(sink.lastAttackEvent.dwAttackerVID == 50);

    TPacketGCItemSet itemSet{};
    itemSet.pos.window_type = 1;
    itemSet.pos.cell = 3;
    itemSet.vnum = 2001;
    itemSet.count = 1;
    bDispatched = dispatcher.DispatchItemSet(itemSet);
    assert(bDispatched);
    assert(sink.lastItemEvent.dwVnum == 2001);

    TPacketGCCharacterDelete charDel{};
    charDel.dwVID = 999;
    bDispatched = dispatcher.DispatchCharacterDelete(charDel);
    assert(bDispatched);
    assert(sink.lastDeadEvent.dwVID == 999);

    // Test uniwersalnego DispatchPacket O(1) z deserializowanym rekordem POD
    bDispatched = dispatcher.DispatchPacket(static_cast<uint8_t>(0x0C), &attack);
    assert(bDispatched);

    bDispatched = dispatcher.DispatchPacket(static_cast<uint16_t>(0x0511), &itemSet);
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

    // 1. Weryfikacja zapobiegania rozlaczeniu klienta (HasHandlerForHeader)
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0401))); // ATTACK
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0410))); // DAMAGE_INFO
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0413))); // CREATE_FLY
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0415))); // DUEL_START
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0510))); // ITEM_DEL
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0511))); // ITEM_SET
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0515))); // ITEM_GROUND_ADD
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0516))); // ITEM_GROUND_DEL
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0205))); // CHARACTER_ADD
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0207))); // CHAR_ADDITIONAL_INFO
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0208))); // CHARACTER_DEL
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0304))); // SYNC_POSITION
    assert(dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0x0B22))); // OBSERVER_MOVE

    // 2. Symulacja odbioru pakietu GC_ATTACK (0x0401) ze 100% integralnoscia danych (Zero Memory Corruption)
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

    // 3. Symulacja odbioru pakietu GC_DAMAGE_INFO (0x0410)
    TPacketGCDamageInfo damagePacket{};
    damagePacket.header = 0x0410;
    damagePacket.length = sizeof(TPacketGCDamageInfo);
    damagePacket.dwVID = 5555;
    damagePacket.damage = 98765;
    damagePacket.flag = 0x04;

    dispatched = dispatcher.DispatchPacket(damagePacket.header, &damagePacket);
    assert(dispatched);
    assert(sink.lastDamageEvent.dwTargetVID == 5555);
    assert(sink.lastDamageEvent.nDamage == 98765);
    assert(sink.lastDamageEvent.byFlag == 0x04);

    // 4. Symulacja odbioru pakietu GC_CHARACTER_DEL (0x0208)
    TPacketGCCharacterDelete delPacket{};
    delPacket.header = 0x0208;
    delPacket.length = sizeof(TPacketGCCharacterDelete);
    delPacket.dwVID = 8888;

    dispatched = dispatcher.DispatchPacket(delPacket.header, &delPacket);
    assert(dispatched);
    assert(sink.lastDeadEvent.dwVID == 8888);

    // 5. Weryfikacja odrzucania nieznanego opkodu bez awarii (zero-crash)
    assert(!dispatcher.HasHandlerForHeader(static_cast<uint16_t>(0xFAFA)));
    assert(!dispatcher.DispatchPacket(static_cast<uint16_t>(0xFAFA), &attackPacket));

    dispatcher.ClearRouters();
    std::cout << "[PASS] Symulacja NetworkStream i weryfikacja framingu x64 zakonczona sukcesem.\n";
}

int main()
{
    std::cout << "========================================================\n";
    std::cout << "URUCHOMIENIE TESTOW: PhaseGame Packet Routers & Dispatcher\n";
    std::cout << "========================================================\n";

    TestNetCombatRouter();
    TestNetItemRouter();
    TestNetActorRouter();
    TestPhaseGamePacketDispatcher();
    TestNetworkStreamBridgeAndFraming();

    std::cout << "\n>>> WSZYSTKIE TESTY ZAKONCZONE SUKCESEM (100% PASS) <<<\n";
    return 0;
}
