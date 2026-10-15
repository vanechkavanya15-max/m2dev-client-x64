#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <vector>
#include <string_view>
#include <cmath>
#include <cstring>

#include "Client/Network/Domain/CombatCommands.h"
#include "Client/Network/Domain/MovementCommands.h"
#include "Client/Network/Domain/ItemCommands.h"
#include "Client/Network/Domain/SelectCommands.h"
#include "Client/Network/Protocol/IServerProtocolDriver.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.h"
#include "Client/Network/Protocol/Drivers/BeaviumProtocolDriver.h"
#include "Client/Network/Protocol/Drivers/MartySamaProtocolDriver.h"
#include "Client/Network/Protocol/Drivers/LuriaProtocolDriver.h"
#include "Client/Network/Protocol/Protocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "Client/Network/Protocol/LuriaProtocol.h"
#include "UserInterface/Contracts/IGameEvents.h"
#include "EterBase/EventBus.h"

// Mock odbiornika zdarzen kontraktowych do weryfikacji DispatchInbound
class MockGameEventSink : public UserInterface::Contracts::IGameEventSink
{
public:
    uint32_t lastAttackerVid{0};
    uint32_t lastVictimVid{0};
    uint8_t  lastMotionType{0};
    bool attackExecutedCalled{false};

    uint32_t lastDeadVid{0};
    bool actorDeadCalled{false};

    uint32_t lastMovedVid{0};
    int32_t  lastMovedX{0};
    int32_t  lastMovedY{0};
    float    lastMovedRot{0.0f};
    bool actorMovedCalled{false};

    uint32_t lastTargetVid{0};
    bool targetChangedCalled{false};

    uint32_t lastItemCell{0};
    uint32_t lastItemVnum{0};
    uint32_t lastItemCount{0};
    bool itemReceivedCalled{false};

    uint32_t lastDamageVictimVid{0};
    uint64_t lastDamageAmount{0};
    uint8_t  lastDamageFlag{0};
    bool damageInfoCalled{false};

    void Reset()
    {
        lastAttackerVid = 0;
        lastVictimVid = 0;
        lastMotionType = 0;
        attackExecutedCalled = false;

        lastDeadVid = 0;
        actorDeadCalled = false;

        lastMovedVid = 0;
        lastMovedX = 0;
        lastMovedY = 0;
        lastMovedRot = 0.0f;
        actorMovedCalled = false;

        lastTargetVid = 0;
        targetChangedCalled = false;

        lastItemCell = 0;
        lastItemVnum = 0;
        lastItemCount = 0;
        itemReceivedCalled = false;

        lastDamageVictimVid = 0;
        lastDamageAmount = 0;
        lastDamageFlag = 0;
        damageInfoCalled = false;
    }

    void OnAttackExecuted(const UserInterface::Contracts::AttackExecutedEvent& event) override
    {
        lastAttackerVid = event.dwAttackerVID;
        lastVictimVid = event.dwVictimVID;
        lastMotionType = event.byMotionType;
        attackExecutedCalled = true;
    }

    void OnActorDead(const UserInterface::Contracts::ActorDeadEvent& event) override
    {
        lastDeadVid = event.dwVID;
        actorDeadCalled = true;
    }

    void OnActorMoved(const UserInterface::Contracts::ActorMovedEvent& event) override
    {
        lastMovedVid = event.dwVID;
        lastMovedX = event.lX;
        lastMovedY = event.lY;
        lastMovedRot = event.fRot;
        actorMovedCalled = true;
    }

    void OnTargetChanged(const UserInterface::Contracts::TargetChangedEvent& event) override
    {
        lastTargetVid = event.dwVID;
        targetChangedCalled = true;
    }

    void OnItemReceived(const UserInterface::Contracts::ItemReceivedEvent& event) override
    {
        lastItemCell = event.dwCell;
        lastItemVnum = event.dwVnum;
        lastItemCount = event.dwCount;
        itemReceivedCalled = true;
    }

    void OnDamageInfo(const UserInterface::Contracts::DamageInfoEvent& event) override
    {
        lastDamageVictimVid = event.dwVictimVID;
        lastDamageAmount = event.dwDamage;
        lastDamageFlag = event.byDamageFlag;
        damageInfoCalled = true;
    }
};

void TestProtocolDriverRegistry()
{
    std::cout << "[Test 1] Testowanie ProtocolDriverRegistry (rejestracja i przelaczanie)..." << std::endl;
    auto& registry = Network::Protocol::ProtocolDriverRegistry::Instance();
    registry.Clear();

    assert(registry.GetActiveDriver() == nullptr);
    assert(!registry.HasDriver("standard_x64"));
    assert(!registry.HasDriver("beavium"));
    assert(!registry.HasDriver("martysama"));
    assert(!registry.HasDriver("luria"));

    // Inicjalizacja domyslnych sterownikow
    registry.InitializeDefaults();
    assert(registry.HasDriver("standard_x64"));
    assert(registry.HasDriver("beavium"));
    assert(registry.HasDriver("martysama"));
    assert(registry.HasDriver("luria"));

    // Domyslnie aktywny jest standard_x64
    auto* activeDriver = registry.GetActiveDriver();
    assert(activeDriver != nullptr);
    assert(activeDriver->GetDriverName() == "standard_x64");
    assert(registry.GetActiveDriverName() == "standard_x64");

    // Dynamiczne przelaczenie na beavium
    bool switchBeavium = registry.SetActiveDriver("beavium");
    assert(switchBeavium);
    assert(registry.GetActiveDriver()->GetDriverName() == "beavium");
    assert(registry.GetActiveDriverName() == "beavium");

    // Dynamiczne przelaczenie na martysama
    bool switchMarty = registry.SetActiveDriver("martysama");
    assert(switchMarty);
    assert(registry.GetActiveDriver()->GetDriverName() == "martysama");
    assert(registry.GetActiveDriverName() == "martysama");

    // Dynamiczne przelaczenie na luria
    bool switchLuria = registry.SetActiveDriver("luria");
    assert(switchLuria);
    assert(registry.GetActiveDriver()->GetDriverName() == "luria");
    assert(registry.GetActiveDriverName() == "luria");

    // Dynamiczne przelaczenie z powrotem na standard_x64
    bool switchX64 = registry.SetActiveDriver("standard_x64");
    assert(switchX64);
    assert(registry.GetActiveDriver()->GetDriverName() == "standard_x64");
    assert(registry.GetActiveDriverName() == "standard_x64");

    // Proba aktywacji nieznanego sterownika
    bool switchUnknown = registry.SetActiveDriver("non_existent");
    assert(!switchUnknown);
    assert(registry.GetActiveDriver()->GetDriverName() == "standard_x64");

    // Bezposrednie pobieranie sterownikow po nazwie
    auto* pLuria = registry.GetDriver("luria");
    assert(pLuria != nullptr);
    assert(pLuria->GetDriverName() == "luria");

    auto* pMarty = registry.GetDriver("martysama");
    assert(pMarty != nullptr);
    assert(pMarty->GetDriverName() == "martysama");

    auto* pBeav = registry.GetDriver("beavium");
    assert(pBeav != nullptr);
    assert(pBeav->GetDriverName() == "beavium");

    auto* pX64 = registry.GetDriver("standard_x64");
    assert(pX64 != nullptr);
    assert(pX64->GetDriverName() == "standard_x64");

    std::cout << "[PASS] ProtocolDriverRegistry dziala prawidlowo!" << std::endl;
}

void TestStandardX64Driver()
{
    std::cout << "[Test 2] Testowanie StandardX64ProtocolDriver..." << std::endl;
    Network::Protocol::Drivers::StandardX64ProtocolDriver driver;

    assert(driver.GetDriverName() == "standard_x64");

    // InspectFrame - za krotki bufor (< 4B)
    uint8_t shortBuf[] = { 0x01, 0x04, 0x0B };
    assert(!driver.InspectFrame(shortBuf).has_value());

    // InspectFrame - niepoprawna dlugosc (< 4B)
    uint8_t badLenBuf[] = { 0x01, 0x04, 0x02, 0x00 };
    assert(!driver.InspectFrame(badLenBuf).has_value());

    // InspectFrame - poprawna ramka 4B: opcode 0x0401, len 11
    uint8_t validFrameBuf[] = { 0x01, 0x04, 0x0B, 0x00, 0x00, 0x00, 0x00 };
    auto inspectValid = driver.InspectFrame(validFrameBuf);
    assert(inspectValid.has_value());
    assert(inspectValid->unifiedOpcode == 0x0401);
    assert(inspectValid->packetLength == 11);
    assert(inspectValid->headerSize == 4);

    // EncodeAttack
    Network::Domain::AttackCommand atkCmd{
        .targetVid = 12345,
        .attackerVid = 1,
        .attackType = 2,
        .sequence = 99,
        .attackMotion = 0
    };
    auto encodedAtk = driver.EncodeAttack(atkCmd);
    assert(encodedAtk.has_value());
    assert(encodedAtk->size() == sizeof(TPacketCGAttack));
    assert(encodedAtk->size() == 11);

    // EncodeMove
    Network::Domain::MoveCommand mvCmd{
        .vid = 1,
        .x = 50000,
        .y = 60000,
        .rotationDegrees = 90.0f,
        .time = 1000,
        .func = 1,
        .arg = 2
    };
    auto encodedMv = driver.EncodeMove(mvCmd);
    assert(encodedMv.has_value());
    assert(encodedMv->size() == sizeof(TPacketCGMove));
    assert(encodedMv->size() == 19);

    // EncodeItemUse
    Network::Domain::ItemUseCommand itemCmd{
        .window = 1,
        .cell = 25
    };
    auto encodedItem = driver.EncodeItemUse(itemCmd);
    assert(encodedItem.has_value());
    assert(encodedItem->size() == sizeof(TPacketCGItemUse));

    // EncodeTarget
    Network::Domain::TargetCommand tgtCmd{
        .targetVid = 998877
    };
    auto encodedTarget = driver.EncodeTarget(tgtCmd);
    assert(encodedTarget.has_value());
    assert(encodedTarget->size() == sizeof(TPacketCGTarget));

    // EncodeItemMove (11B)
    Network::Domain::ItemMoveCommand moveCmd{
        .srcWindow = 1,
        .srcCell = 10,
        .dstWindow = 1,
        .dstCell = 20,
        .count = 5
    };
    auto encodedMove = driver.EncodeItemMove(moveCmd);
    assert(encodedMove.has_value());
    assert(encodedMove->size() == sizeof(TPacketCGItemMove));
    assert(encodedMove->size() == 11);

    // EncodeItemDrop (12B)
    Network::Domain::ItemDropCommand dropCmd{
        .window = 1,
        .cell = 10,
        .count = 500
    };
    auto encodedDrop = driver.EncodeItemDrop(dropCmd);
    assert(encodedDrop.has_value());
    assert(encodedDrop->size() == sizeof(TPacketCGItemDrop2));
    assert(encodedDrop->size() == 12);

    // EncodeItemPickUp (8B)
    Network::Domain::ItemPickUpCommand pickupCmd{
        .vid = 8888
    };
    auto encodedPickup = driver.EncodeItemPickUp(pickupCmd);
    assert(encodedPickup.has_value());
    assert(encodedPickup->size() == sizeof(TPacketCGItemPickUp));
    assert(encodedPickup->size() == 8);

    // EncodeSelectEmpire (5B)
    Network::Domain::EmpireSelectCommand empireCmd{
        .empireId = 2
    };
    auto encodedEmpire = driver.EncodeSelectEmpire(empireCmd);
    assert(encodedEmpire.has_value());
    assert(encodedEmpire->size() == sizeof(TPacketCGEmpire));
    assert(encodedEmpire->size() == 5);

    // EncodeSelectCharacter (5B)
    Network::Domain::CharacterSelectCommand charSelCmd{
        .slotIndex = 1
    };
    auto encodedCharSel = driver.EncodeSelectCharacter(charSelCmd);
    assert(encodedCharSel.has_value());
    assert(encodedCharSel->size() == sizeof(TPacketCGSelectCharacter));
    assert(encodedCharSel->size() == 5);

    // EncodeCreateCharacter
    Network::Domain::CharacterCreateCommand charCreateCmd{
        .slotIndex = 0,
        .name = "WarriorX",
        .job = 0,
        .shape = 1,
        .con = 10,
        .int_ = 5,
        .str = 15,
        .dex = 8
    };
    auto encodedCharCreate = driver.EncodeCreateCharacter(charCreateCmd);
    assert(encodedCharCreate.has_value());
    assert(encodedCharCreate->size() == sizeof(TPacketCGCreateCharacter));

    // EncodeDestroyCharacter
    Network::Domain::CharacterDestroyCommand charDestroyCmd{
        .slotIndex = 0,
        .privateCode = "1234567"
    };
    auto encodedCharDestroy = driver.EncodeDestroyCharacter(charDestroyCmd);
    assert(encodedCharDestroy.has_value());
    assert(encodedCharDestroy->size() == sizeof(TPacketCGDestroyCharacter));

    // DispatchInbound - pusty bufor
    assert(!driver.DispatchInbound(0x0401, {}, nullptr));

    std::cout << "[PASS] StandardX64ProtocolDriver dziala prawidlowo!" << std::endl;
}

void TestBeaviumDriver()
{
    std::cout << "[Test 3] Testowanie BeaviumProtocolDriver..." << std::endl;
    Network::Protocol::Drivers::BeaviumProtocolDriver driver;

    assert(driver.GetDriverName() == "beavium");

    // InspectFrame - pusty bufor
    assert(!driver.InspectFrame({}).has_value());

    // InspectFrame - fallback 2B dla opkodu spoza glownej tabeli (analiza disasemblera Beavium)
    uint8_t unknownBuf[] = { 0xFE, 0x00, 0x00 };
    auto inspectUnknown = driver.InspectFrame(unknownBuf);
    assert(inspectUnknown.has_value());
    assert(inspectUnknown->unifiedOpcode == 0xFE);
    assert(inspectUnknown->packetLength == 2);
    assert(inspectUnknown->headerSize == 1);

    // InspectFrame - staly pakiet GC::ATTACK (0x0C, rozmiar 12B)
    uint8_t fixedAtkBuf[] = { Beavium::GC::ATTACK, 0x01, 0x02 };
    auto inspectAtk = driver.InspectFrame(fixedAtkBuf);
    assert(inspectAtk.has_value());
    assert(inspectAtk->unifiedOpcode == 0x0C);
    assert(inspectAtk->packetLength == 12);
    assert(inspectAtk->headerSize == 1);

    // InspectFrame - dynamiczny pakiet GC::CHAT (0x04, rozmiar 32B, bazowy naglowek Beavium >= 14B)
    std::vector<uint8_t> dynChatBuf(32, 0);
    dynChatBuf[0] = Beavium::GC::CHAT;
    dynChatBuf[1] = 0x20;
    dynChatBuf[2] = 0x00;
    auto inspectChat = driver.InspectFrame(dynChatBuf);
    assert(inspectChat.has_value());
    assert(inspectChat->unifiedOpcode == 0x04);
    assert(inspectChat->packetLength == 32);
    assert(inspectChat->headerSize == 1);

    // EncodeAttack - ProxyPacketCGAttack (8B)
    Network::Domain::AttackCommand atkCmd{
        .targetVid = 77777,
        .attackerVid = 1,
        .attackType = 0,
        .sequence = 1234,
        .attackMotion = 5
    };
    auto encodedAtk = driver.EncodeAttack(atkCmd);
    assert(encodedAtk.has_value());
    assert(encodedAtk->size() == 8);
    assert((*encodedAtk)[0] == Beavium::CG::ATTACK);

    // EncodeMove - Beavium TPacketCGMoveBeavium (24B)
    Network::Domain::MoveCommand mvCmd{
        .vid = 1,
        .x = 100000,
        .y = 200000,
        .rotationDegrees = 180.0f,
        .time = 5000,
        .func = 1,
        .arg = 15
    };
    auto encodedMv = driver.EncodeMove(mvCmd);
    assert(encodedMv.has_value());
    assert(encodedMv->size() == sizeof(Beavium::TPacketCGMoveBeavium));
    assert(encodedMv->size() == 29);

    // EncodeItemUse (12B)
    Network::Domain::ItemUseCommand itemCmd{
        .window = 1,
        .cell = 12
    };
    auto encodedItem = driver.EncodeItemUse(itemCmd);
    assert(encodedItem.has_value());
    assert(encodedItem->size() == sizeof(Beavium::TPacketCGItemUseBeavium));
    assert(encodedItem->size() == 12);

    // EncodeTarget (5B)
    Network::Domain::TargetCommand tgtCmd{
        .targetVid = 445566
    };
    auto encodedTarget = driver.EncodeTarget(tgtCmd);
    assert(encodedTarget.has_value());
    assert(encodedTarget->size() == sizeof(Beavium::TPacketCGTargetBeavium));
    assert(encodedTarget->size() == 5);

    // EncodeItemMove (8B)
    Network::Domain::ItemMoveCommand moveCmd{
        .srcWindow = 1,
        .srcCell = 10,
        .dstWindow = 1,
        .dstCell = 20,
        .count = 5
    };
    auto encodedMove = driver.EncodeItemMove(moveCmd);
    assert(encodedMove.has_value());
    assert(encodedMove->size() == sizeof(Beavium::TPacketCGItemMoveBeavium));
    assert(encodedMove->size() == 8);
    assert((*encodedMove)[0] == Beavium::CG::ITEM_MOVE);

    // EncodeItemDrop (9B)
    Network::Domain::ItemDropCommand dropCmd{
        .window = 1,
        .cell = 10,
        .count = 500
    };
    auto encodedDrop = driver.EncodeItemDrop(dropCmd);
    assert(encodedDrop.has_value());
    assert(encodedDrop->size() == sizeof(Beavium::TPacketCGItemDrop2Beavium));
    assert(encodedDrop->size() == 9);

    // EncodeItemPickUp (5B)
    Network::Domain::ItemPickUpCommand pickupCmd{
        .vid = 8888
    };
    auto encodedPickup = driver.EncodeItemPickUp(pickupCmd);
    assert(encodedPickup.has_value());
    assert(encodedPickup->size() == sizeof(Beavium::TPacketCGItemPickupBeavium));
    assert(encodedPickup->size() == 5);
    assert((*encodedPickup)[0] == Beavium::CG::ITEM_PICKUP);

    // EncodeSelectEmpire (2B)
    Network::Domain::EmpireSelectCommand empireCmd{
        .empireId = 2
    };
    auto encodedEmpire = driver.EncodeSelectEmpire(empireCmd);
    assert(encodedEmpire.has_value());
    assert(encodedEmpire->size() == sizeof(Beavium::TPacketCGSelectEmpireBeavium));
    assert(encodedEmpire->size() == 2);
    assert((*encodedEmpire)[0] == Beavium::CG::SELECT_EMPIRE);

    // EncodeSelectCharacter (14B)
    Network::Domain::CharacterSelectCommand charSelCmd{
        .slotIndex = 1
    };
    auto encodedCharSel = driver.EncodeSelectCharacter(charSelCmd);
    assert(encodedCharSel.has_value());
    assert(encodedCharSel->size() == sizeof(Beavium::TPacketCGCharSelectBeavium));
    assert(encodedCharSel->size() == 14);
    assert((*encodedCharSel)[0] == Beavium::CG::CHAR_SELECT);

    // EncodeCreateCharacter (42B)
    Network::Domain::CharacterCreateCommand charCreateCmd{
        .slotIndex = 0,
        .name = "WarriorX",
        .job = 0,
        .shape = 1,
        .con = 10,
        .int_ = 5,
        .str = 15,
        .dex = 8
    };
    auto encodedCharCreate = driver.EncodeCreateCharacter(charCreateCmd);
    assert(encodedCharCreate.has_value());
    assert(encodedCharCreate->size() == sizeof(Beavium::TPacketCGCharacterCreateBeavium));
    assert(encodedCharCreate->size() == 42);
    assert((*encodedCharCreate)[0] == Beavium::CG::CHARACTER_CREATE);

    // EncodeDestroyCharacter (10B)
    Network::Domain::CharacterDestroyCommand charDestroyCmd{
        .slotIndex = 0,
        .privateCode = "1234567"
    };
    auto encodedCharDestroy = driver.EncodeDestroyCharacter(charDestroyCmd);
    assert(encodedCharDestroy.has_value());
    assert(encodedCharDestroy->size() == sizeof(Beavium::TPacketCGCharacterDestroyBeavium));
    assert(encodedCharDestroy->size() == 10);
    assert((*encodedCharDestroy)[0] == Beavium::CG::CHARACTER_DESTROY);

    // DispatchInbound z MockGameEventSink
    MockGameEventSink mockSink;

    // Dispatch GC::ATTACK (0x0C)
    Beavium::TPacketGCAttackBeavium gcAtk{};
    gcAtk.header = Beavium::GC::ATTACK;
    gcAtk.dwVID = 55;
    gcAtk.dwVictimVID = 99;
    gcAtk.bType = 3;
    gcAtk.wMotionArg = 0;
    std::span<const uint8_t> atkSpan(reinterpret_cast<const uint8_t*>(&gcAtk), sizeof(gcAtk));

    bool atkRes = driver.DispatchInbound(Beavium::GC::ATTACK, atkSpan, &mockSink);
    assert(atkRes);
    assert(mockSink.attackExecutedCalled);
    assert(mockSink.lastAttackerVid == 55);
    assert(mockSink.lastVictimVid == 99);
    assert(mockSink.lastMotionType == 3);

    // Dispatch GC::DEAD (0x0E)
    mockSink.Reset();
    Beavium::TPacketGCDeadBeavium gcDead{};
    gcDead.header = Beavium::GC::DEAD;
    gcDead.dwVID = 444;
    std::span<const uint8_t> deadSpan(reinterpret_cast<const uint8_t*>(&gcDead), sizeof(gcDead));

    bool deadRes = driver.DispatchInbound(Beavium::GC::DEAD, deadSpan, &mockSink);
    assert(deadRes);
    assert(mockSink.actorDeadCalled);
    assert(mockSink.lastDeadVid == 444);

    std::cout << "[PASS] BeaviumProtocolDriver dziala prawidlowo!" << std::endl;
}

void TestMartySamaDriver()
{
    std::cout << "[Test 4] Testowanie MartySamaProtocolDriver..." << std::endl;
    Network::Protocol::Drivers::MartySamaProtocolDriver driver;

    assert(driver.GetDriverName() == "martysama");

    // 1. InspectFrame - testy brzegowe i stale pakiety
    assert(!driver.InspectFrame({}).has_value()); // Pusty bufor

    uint8_t unknownBuf[] = { 0xBA, 0x00, 0x00 };
    assert(!driver.InspectFrame(unknownBuf).has_value()); // Nieznany opkod

    // Staly pakiet HEADER_GC_ATTACK (12 / 0x0C, rozmiar 10B)
    uint8_t fixedAtkBuf[] = { 12, 0x01, 0x02 };
    auto inspectAtk = driver.InspectFrame(fixedAtkBuf);
    assert(inspectAtk.has_value());
    assert(inspectAtk->unifiedOpcode == 12);
    assert(inspectAtk->packetLength == 10);
    assert(inspectAtk->headerSize == 1);

    // Staly pakiet HEADER_GC_DEAD (14 / 0x0E, rozmiar 5B)
    uint8_t fixedDeadBuf[] = { 14 };
    auto inspectDead = driver.InspectFrame(fixedDeadBuf);
    assert(inspectDead.has_value());
    assert(inspectDead->unifiedOpcode == 14);
    assert(inspectDead->packetLength == 5);
    assert(inspectDead->headerSize == 1);

    // Staly pakiet HEADER_GC_MOVE (3 / 0x03, rozmiar 24B)
    uint8_t fixedMvBuf[] = { 3 };
    auto inspectMv = driver.InspectFrame(fixedMvBuf);
    assert(inspectMv.has_value());
    assert(inspectMv->unifiedOpcode == 3);
    assert(inspectMv->packetLength == 24);
    assert(inspectMv->headerSize == 1);

    // Staly pakiet HEADER_GC_TARGET (63 / 0x3F, rozmiar 6B)
    uint8_t fixedTgtBuf[] = { 63 };
    auto inspectTgt = driver.InspectFrame(fixedTgtBuf);
    assert(inspectTgt.has_value());
    assert(inspectTgt->unifiedOpcode == 63);
    assert(inspectTgt->packetLength == 6);
    assert(inspectTgt->headerSize == 1);

    // Staly pakiet HEADER_GC_DAMAGE_INFO (135 / 0x87, rozmiar 10B)
    uint8_t fixedDmgBuf[] = { 135 };
    auto inspectDmg = driver.InspectFrame(fixedDmgBuf);
    assert(inspectDmg.has_value());
    assert(inspectDmg->unifiedOpcode == 135);
    assert(inspectDmg->packetLength == 10);
    assert(inspectDmg->headerSize == 1);

    // Staly pakiet HEADER_GC_ITEM_SET (21 / 0x15, rozmiar 51B)
    uint8_t fixedItemSetBuf[] = { 21 };
    auto inspectItemSet = driver.InspectFrame(fixedItemSetBuf);
    assert(inspectItemSet.has_value());
    assert(inspectItemSet->unifiedOpcode == 21);
    assert(inspectItemSet->packetLength == 51);
    assert(inspectItemSet->headerSize == 1);

    // Staly pakiet HEADER_GC_HANDSHAKE (255 / 0xFF, rozmiar 13B)
    uint8_t fixedHandshakeBuf[] = { 255 };
    auto inspectHandshake = driver.InspectFrame(fixedHandshakeBuf);
    assert(inspectHandshake.has_value());
    assert(inspectHandshake->unifiedOpcode == 255);
    assert(inspectHandshake->packetLength == 13);
    assert(inspectHandshake->headerSize == 1);

    // 2. InspectFrame - pakiety dynamiczne z 2-bajtowym naglowkiem dlugosci po opkodzie
    // Za krotki bufor dynamiczny (< 3B)
    uint8_t shortDynBuf[] = { 0x04, 0x10 };
    assert(!driver.InspectFrame(shortDynBuf).has_value());

    // Dynamiczny HEADER_GC_CHAT (4): dlugosc 37 bajtow (0x0025)
    uint8_t dynChatBuf[] = { 0x04, 0x25, 0x00, 0xAA, 0xBB };
    auto inspectDynChat = driver.InspectFrame(dynChatBuf);
    assert(inspectDynChat.has_value());
    assert(inspectDynChat->unifiedOpcode == 4);
    assert(inspectDynChat->packetLength == 37);
    assert(inspectDynChat->headerSize == 1);

    // Dynamiczny HEADER_GC_SCRIPT (45 / 0x2D): dlugosc 64 bajty (0x0040)
    uint8_t dynScriptBuf[] = { 0x2D, 0x40, 0x00 };
    auto inspectDynScript = driver.InspectFrame(dynScriptBuf);
    assert(inspectDynScript.has_value());
    assert(inspectDynScript->unifiedOpcode == 45);
    assert(inspectDynScript->packetLength == 64);
    assert(inspectDynScript->headerSize == 1);

    // Dynamiczny HEADER_GC_QUEST_INFO (81 / 0x51): dlugosc 50 bajtow (0x0032)
    uint8_t dynQuestBuf[] = { 0x51, 0x32, 0x00 };
    auto inspectDynQuest = driver.InspectFrame(dynQuestBuf);
    assert(inspectDynQuest.has_value());
    assert(inspectDynQuest->unifiedOpcode == 81);
    assert(inspectDynQuest->packetLength == 50);
    assert(inspectDynQuest->headerSize == 1);

    // Dynamiczny HEADER_GC_WHISPER (34 / 0x22): dlugosc 30 bajtow (0x001E)
    uint8_t dynWhisperBuf[] = { 0x22, 0x1E, 0x00 };
    auto inspectDynWhisper = driver.InspectFrame(dynWhisperBuf);
    assert(inspectDynWhisper.has_value());
    assert(inspectDynWhisper->unifiedOpcode == 34);
    assert(inspectDynWhisper->packetLength == 30);
    assert(inspectDynWhisper->headerSize == 1);

    // 3. EncodeAttack - TPacketCGAttackMarty (8B: opcode 2, bType, dwVictimVID, CRC)
    Network::Domain::AttackCommand atkCmd{
        .targetVid = 45678,
        .attackerVid = 1,
        .attackType = 1,
        .sequence = 10,
        .attackMotion = 0
    };
    auto encodedAtk = driver.EncodeAttack(atkCmd);
    assert(encodedAtk.has_value());
    assert(encodedAtk->size() == 8);
    assert((*encodedAtk)[0] == 2); // HEADER_CG_ATTACK
    assert((*encodedAtk)[1] == 1); // bType
    uint32_t encVictimVid = 0;
    std::memcpy(&encVictimVid, encodedAtk->data() + 2, sizeof(uint32_t));
    assert(encVictimVid == 45678);

    // 4. EncodeMove - TPacketCGMoveMarty (16B: opcode 7, bFunc, bArg, bRot, lX, lY, dwTime)
    Network::Domain::MoveCommand mvCmd{
        .vid = 1,
        .x = 12345,
        .y = 67890,
        .rotationDegrees = 90.0f,
        .time = 4000,
        .func = 1,
        .arg = 0
    };
    auto encodedMv = driver.EncodeMove(mvCmd);
    assert(encodedMv.has_value());
    assert(encodedMv->size() == 16);
    assert((*encodedMv)[0] == 7); // HEADER_CG_MOVE
    assert((*encodedMv)[1] == 1); // bFunc
    assert((*encodedMv)[2] == 0); // bArg
    assert((*encodedMv)[3] == static_cast<uint8_t>(90.0f / 5.0f)); // 18 segmentow
    int32_t encX = 0, encY = 0;
    uint32_t encTime = 0;
    std::memcpy(&encX, encodedMv->data() + 4, sizeof(int32_t));
    std::memcpy(&encY, encodedMv->data() + 8, sizeof(int32_t));
    std::memcpy(&encTime, encodedMv->data() + 12, sizeof(uint32_t));
    assert(encX == 12345);
    assert(encY == 67890);
    assert(encTime == 4000);

    // 5. EncodeItemUse - TPacketCGItemUseMarty (4B: opcode 11, window, cell)
    Network::Domain::ItemUseCommand itemCmd{
        .window = 1, // INVENTORY
        .cell = 25
    };
    auto encodedItem = driver.EncodeItemUse(itemCmd);
    assert(encodedItem.has_value());
    assert(encodedItem->size() == 4);
    assert((*encodedItem)[0] == 11); // HEADER_CG_ITEM_USE
    assert((*encodedItem)[1] == 1);  // window_type
    uint16_t encCell = 0;
    std::memcpy(&encCell, encodedItem->data() + 2, sizeof(uint16_t));
    assert(encCell == 25);

    // 6. EncodeTarget - TPacketCGTargetMarty (5B: opcode 61, targetVid)
    Network::Domain::TargetCommand tgtCmd{
        .targetVid = 98765
    };
    auto encodedTarget = driver.EncodeTarget(tgtCmd);
    assert(encodedTarget.has_value());
    assert(encodedTarget->size() == 5);
    assert((*encodedTarget)[0] == 61); // HEADER_CG_TARGET
    uint32_t encTargetVid = 0;
    std::memcpy(&encTargetVid, encodedTarget->data() + 1, sizeof(uint32_t));
    assert(encTargetVid == 98765);

    // EncodeItemMove (8B: opcode 13)
    Network::Domain::ItemMoveCommand moveCmd{
        .srcWindow = 1,
        .srcCell = 10,
        .dstWindow = 1,
        .dstCell = 20,
        .count = 5
    };
    auto encodedMove = driver.EncodeItemMove(moveCmd);
    assert(encodedMove.has_value());
    assert(encodedMove->size() == 8);
    assert((*encodedMove)[0] == 13);

    // EncodeItemDrop (9B: opcode 20)
    Network::Domain::ItemDropCommand dropCmd{
        .window = 1,
        .cell = 10,
        .count = 500
    };
    auto encodedDrop = driver.EncodeItemDrop(dropCmd);
    assert(encodedDrop.has_value());
    assert(encodedDrop->size() == 9);
    assert((*encodedDrop)[0] == 20);

    // EncodeItemPickUp (5B: opcode 15)
    Network::Domain::ItemPickUpCommand pickupCmd{
        .vid = 8888
    };
    auto encodedPickup = driver.EncodeItemPickUp(pickupCmd);
    assert(encodedPickup.has_value());
    assert(encodedPickup->size() == 5);
    assert((*encodedPickup)[0] == 15);

    // EncodeSelectEmpire (2B: opcode 90)
    Network::Domain::EmpireSelectCommand empireCmd{
        .empireId = 2
    };
    auto encodedEmpire = driver.EncodeSelectEmpire(empireCmd);
    assert(encodedEmpire.has_value());
    assert(encodedEmpire->size() == 2);
    assert((*encodedEmpire)[0] == 90);

    // EncodeSelectCharacter (2B: opcode 6)
    Network::Domain::CharacterSelectCommand charSelCmd{
        .slotIndex = 1
    };
    auto encodedCharSel = driver.EncodeSelectCharacter(charSelCmd);
    assert(encodedCharSel.has_value());
    assert(encodedCharSel->size() == 2);
    assert((*encodedCharSel)[0] == 6);

    // EncodeCreateCharacter (34B: opcode 4)
    Network::Domain::CharacterCreateCommand charCreateCmd{
        .slotIndex = 0,
        .name = "WarriorX",
        .job = 0,
        .shape = 1,
        .con = 10,
        .int_ = 5,
        .str = 15,
        .dex = 8
    };
    auto encodedCharCreate = driver.EncodeCreateCharacter(charCreateCmd);
    assert(encodedCharCreate.has_value());
    assert(encodedCharCreate->size() == 34);
    assert((*encodedCharCreate)[0] == 4);

    // EncodeDestroyCharacter (10B: opcode 5)
    Network::Domain::CharacterDestroyCommand charDestroyCmd{
        .slotIndex = 0,
        .privateCode = "1234567"
    };
    auto encodedCharDestroy = driver.EncodeDestroyCharacter(charDestroyCmd);
    assert(encodedCharDestroy.has_value());
    assert(encodedCharDestroy->size() == 10);
    assert((*encodedCharDestroy)[0] == 5);

    // 7. DispatchInbound - mapowanie na MockGameEventSink
    MockGameEventSink mockSink;

    // Dispatch ATTACK (12)
    #pragma pack(push, 1)
    struct GCAttackTest {
        uint8_t header{12};
        uint32_t dwVID{101};
        uint32_t dwVictimVID{202};
        uint8_t bType{2};
    } atkPacket;
    #pragma pack(pop)
    std::span<const uint8_t> atkSpan(reinterpret_cast<const uint8_t*>(&atkPacket), sizeof(atkPacket));

    bool atkRes = driver.DispatchInbound(12, atkSpan, &mockSink);
    assert(atkRes);
    assert(mockSink.attackExecutedCalled);
    assert(mockSink.lastAttackerVid == 101);
    assert(mockSink.lastVictimVid == 202);
    assert(mockSink.lastMotionType == 2);

    // Dispatch DEAD (14)
    mockSink.Reset();
    #pragma pack(push, 1)
    struct GCDeadTest {
        uint8_t header{14};
        uint32_t dwVID{303};
    } deadPacket;
    #pragma pack(pop)
    std::span<const uint8_t> deadSpan(reinterpret_cast<const uint8_t*>(&deadPacket), sizeof(deadPacket));

    bool deadRes = driver.DispatchInbound(14, deadSpan, &mockSink);
    assert(deadRes);
    assert(mockSink.actorDeadCalled);
    assert(mockSink.lastDeadVid == 303);

    // Dispatch MOVE (3)
    mockSink.Reset();
    #pragma pack(push, 1)
    struct GCMoveTest {
        uint8_t header{3};
        uint8_t bFunc{1};
        uint8_t bArg{0};
        uint8_t bRot{18}; // 90 stopni
        uint32_t dwVID{404};
        int32_t lX{1000};
        int32_t lY{2000};
        uint32_t dwTime{5000};
        uint32_t dwDuration{0};
    } mvPacket;
    #pragma pack(pop)
    std::span<const uint8_t> mvSpan(reinterpret_cast<const uint8_t*>(&mvPacket), sizeof(mvPacket));

    bool mvRes = driver.DispatchInbound(3, mvSpan, &mockSink);
    assert(mvRes);
    assert(mockSink.actorMovedCalled);
    assert(mockSink.lastMovedVid == 404);
    assert(mockSink.lastMovedX == 1000);
    assert(mockSink.lastMovedY == 2000);
    assert(std::fabs(mockSink.lastMovedRot - 90.0f) < 0.001f);

    // Dispatch TARGET (63)
    mockSink.Reset();
    #pragma pack(push, 1)
    struct GCTargetTest {
        uint8_t header{63};
        uint32_t dwVID{505};
        uint8_t bHPPercent{75};
    } tgtPacket;
    #pragma pack(pop)
    std::span<const uint8_t> tgtSpan(reinterpret_cast<const uint8_t*>(&tgtPacket), sizeof(tgtPacket));

    bool tgtRes = driver.DispatchInbound(63, tgtSpan, &mockSink);
    assert(tgtRes);
    assert(mockSink.targetChangedCalled);
    assert(mockSink.lastTargetVid == 505);

    // Dispatch ITEM_SET (21)
    mockSink.Reset();
    #pragma pack(push, 1)
    struct GCItemSetTest {
        uint8_t header{21};
        uint8_t window{1};
        uint16_t cell{12};
        uint32_t vnum{10};
        uint8_t count{5};
    } itemSetPacket;
    #pragma pack(pop)
    std::span<const uint8_t> itemSetSpan(reinterpret_cast<const uint8_t*>(&itemSetPacket), sizeof(itemSetPacket));

    bool itemRes = driver.DispatchInbound(21, itemSetSpan, &mockSink);
    assert(itemRes);
    assert(mockSink.itemReceivedCalled);
    assert(mockSink.lastItemCell == 12);
    assert(mockSink.lastItemVnum == 10);
    assert(mockSink.lastItemCount == 5);

    // Dispatch DAMAGE_INFO (135)
    mockSink.Reset();
    #pragma pack(push, 1)
    struct GCDamageTest {
        uint8_t header{135};
        uint32_t dwVID{606};
        uint8_t flag{1};
        int32_t damage{1500};
    } dmgPacket;
    #pragma pack(pop)
    std::span<const uint8_t> dmgSpan(reinterpret_cast<const uint8_t*>(&dmgPacket), sizeof(dmgPacket));

    bool dmgRes = driver.DispatchInbound(135, dmgSpan, &mockSink);
    assert(dmgRes);
    assert(mockSink.damageInfoCalled);
    assert(mockSink.lastDamageVictimVid == 606);
    assert(mockSink.lastDamageAmount == 1500);
    assert(mockSink.lastDamageFlag == 1);

    // Dispatch pustego bufora zwraca false
    assert(!driver.DispatchInbound(12, {}, &mockSink));

    std::cout << "[PASS] MartySamaProtocolDriver dziala prawidlowo!" << std::endl;
}

void TestPolymorphicDrivers()
{
    std::cout << "[Test 5] Testowanie polimorficzne wszystkich sterownikow przez IServerProtocolDriver*..." << std::endl;
    auto& registry = Network::Protocol::ProtocolDriverRegistry::Instance();
    registry.InitializeDefaults();

    std::vector<std::string_view> driverNames = { "standard_x64", "beavium", "martysama", "luria" };

    Network::Domain::AttackCommand atkCmd{ .targetVid = 100, .attackType = 0 };
    Network::Domain::MoveCommand mvCmd{ .vid = 100, .x = 10, .y = 20, .rotationDegrees = 0.0f, .time = 100, .func = 1 };
    Network::Domain::ItemUseCommand itemCmd{ .window = 1, .cell = 0 };
    Network::Domain::TargetCommand tgtCmd{ .targetVid = 200 };

    for (auto name : driverNames)
    {
        auto* pDriver = registry.GetDriver(name);
        assert(pDriver != nullptr);
        assert(pDriver->GetDriverName() == name);

        // EncodeAttack
        auto resAtk = pDriver->EncodeAttack(atkCmd);
        assert(resAtk.has_value());
        assert(!resAtk->empty());

        // EncodeMove
        auto resMv = pDriver->EncodeMove(mvCmd);
        assert(resMv.has_value());
        assert(!resMv->empty());

        // EncodeItemUse
        auto resItem = pDriver->EncodeItemUse(itemCmd);
        assert(resItem.has_value());
        assert(!resItem->empty());

        // EncodeTarget
        auto resTgt = pDriver->EncodeTarget(tgtCmd);
        assert(resTgt.has_value());
        assert(!resTgt->empty());

        // EncodeItemMove
        Network::Domain::ItemMoveCommand moveCmd{ .srcWindow = 1, .srcCell = 10, .dstWindow = 1, .dstCell = 20, .count = 5 };
        auto resMove = pDriver->EncodeItemMove(moveCmd);
        assert(resMove.has_value());
        assert(!resMove->empty());

        // EncodeItemDrop
        Network::Domain::ItemDropCommand dropCmd{ .window = 1, .cell = 10, .count = 500 };
        auto resDrop = pDriver->EncodeItemDrop(dropCmd);
        assert(resDrop.has_value());
        assert(!resDrop->empty());

        // EncodeItemPickUp
        Network::Domain::ItemPickUpCommand pickupCmd{ .vid = 8888 };
        auto resPickup = pDriver->EncodeItemPickUp(pickupCmd);
        assert(resPickup.has_value());
        assert(!resPickup->empty());

        // EncodeSelectEmpire
        Network::Domain::EmpireSelectCommand empireCmd{ .empireId = 2 };
        auto resEmpire = pDriver->EncodeSelectEmpire(empireCmd);
        assert(resEmpire.has_value());
        assert(!resEmpire->empty());

        // EncodeSelectCharacter
        Network::Domain::CharacterSelectCommand charSelCmd{ .slotIndex = 1 };
        auto resCharSel = pDriver->EncodeSelectCharacter(charSelCmd);
        assert(resCharSel.has_value());
        assert(!resCharSel->empty());

        // EncodeCreateCharacter
        Network::Domain::CharacterCreateCommand charCreateCmd{ .slotIndex = 0, .name = "Hero", .job = 0, .shape = 1, .con = 10, .int_ = 5, .str = 15, .dex = 8 };
        auto resCharCreate = pDriver->EncodeCreateCharacter(charCreateCmd);
        assert(resCharCreate.has_value());
        assert(!resCharCreate->empty());

        // EncodeDestroyCharacter
        Network::Domain::CharacterDestroyCommand charDestroyCmd{ .slotIndex = 0, .privateCode = "1234567" };
        auto resCharDestroy = pDriver->EncodeDestroyCharacter(charDestroyCmd);
        assert(resCharDestroy.has_value());
        assert(!resCharDestroy->empty());

        // EncodeEnterGame - wejscie do gry
        auto resEnterGame = pDriver->EncodeEnterGame();
        assert(resEnterGame.has_value());
        assert(!resEnterGame->empty());

        // InspectFrame - pusty bufor zawsze daje nullopt
        assert(!pDriver->InspectFrame({}).has_value());

        // Minimalne interwaly czasowe
        assert(pDriver->GetMinAttackIntervalMs() > 0);
        assert(pDriver->GetMinActionIntervalMs() > 0);
    }

    std::cout << "[PASS] Test polimorficzny dla standard_x64, beavium i martysama zakonczony sukcesem!" << std::endl;
}

void TestLuriaProtocolDefinition()
{
    std::cout << "[TEST] Weryfikacja struktur i tabeli protokolu Lurii..." << std::endl;

    // 1. Static asserty i rozmiary struktur
    assert(sizeof(Luria::TPacketCGAttackLuria) == 8);
    assert(sizeof(Luria::TPacketCGMoveLuria) == 19);
    assert(sizeof(Luria::TPacketCGClientVersionLuria) == 67);
    assert(sizeof(Luria::TPacketCGItemUseLuria) == 4);
    assert(sizeof(Luria::TPacketCGItemDropLuria) == 8);
    assert(sizeof(Luria::TPacketCGItemMoveLuria) == 9);
    assert(sizeof(Luria::TPacketCGItemPickUpLuria) == 5);
    assert(sizeof(Luria::TPacketCGSelectCharacterLuria) == 2);
    assert(sizeof(Luria::TPacketCGEmpireLuria) == 2);
    assert(sizeof(Luria::TPacketCGDestroyCharacterLuria) == 10);
    assert(sizeof(Luria::TPacketCGCreateCharacterLuria) == 34);
    assert(sizeof(Luria::TPacketGCWarpLuria) == 15);
    assert(sizeof(Luria::TPacketCGTargetLuria) == 5);
    assert(sizeof(Luria::TPacketCGHandshakeLuria) == 13);
    assert(sizeof(Luria::TPacketCGKeyAgreementLuria) == 261);

    // 2. XOR CRC table i funkcja GetLuriaAttackProcPiece
    for (uint32_t seq = 0; seq < 16; ++seq)
    {
        assert(Luria::GetLuriaAttackProcPiece(seq) == Luria::CRC_XOR_TABLE[seq % 8]);
    }

    // 3. Tabela GetGCInfo
    Luria::LuriaGCInfo info{};

    // Pakiety stale (type 0)
    assert(Luria::GetGCInfo(0x01, info) && info.size == 37 && !info.isDynamic);
    assert(Luria::GetGCInfo(0x02, info) && info.size == 5 && !info.isDynamic);
    assert(Luria::GetGCInfo(0x03, info) && info.size == 24 && !info.isDynamic);
    assert(Luria::GetGCInfo(0x06, info) && info.size == 297 && !info.isDynamic);
    assert(Luria::GetGCInfo(0x20, info) && info.size == 489 && !info.isDynamic);
    assert(Luria::GetGCInfo(0xFF, info) && info.size == 13 && !info.isDynamic);

    // Pakiety dynamiczne 16-bit (type 1)
    assert(Luria::GetGCInfo(0x04, info) && info.size == 9 && info.isDynamic);
    assert(Luria::GetGCInfo(0x05, info) && info.size == 3 && info.isDynamic);
    assert(Luria::GetGCInfo(0x26, info) && info.size == 4 && info.isDynamic);
    assert(Luria::GetGCInfo(0x28, info) && info.size == 3 && info.isDynamic);

    // Pakiet 0xBE (type 2 - 32-bit len)
    assert(Luria::GetGCInfo(0xBE, info) && info.size == 5 && info.isDynamic);

    // Default fallback
    assert(Luria::GetGCInfo(0xEE, info) && info.size == 2 && !info.isDynamic);

    std::cout << "[PASS] Protokol Lurii (LuriaProtocol.h + luria_gc_table.inl) zweryfikowany pomyslnie!" << std::endl;
}

void TestLuriaDriver()
{
    std::cout << "[Test 6] Testowanie LuriaProtocolDriver..." << std::endl;
    Network::Protocol::Drivers::LuriaProtocolDriver driver;

    assert(driver.GetDriverName() == "luria");

    // 1. InspectFrame - testy brzegowe i stale pakiety
    assert(!driver.InspectFrame({}).has_value()); // Pusty bufor

    // Staly pakiet GC::CHARACTER_MOVE (3 / 0x03, 24B)
    uint8_t fixedMvBuf[] = { Luria::GC::CHARACTER_MOVE, 0x01, 0x02 };
    auto inspectMv = driver.InspectFrame(fixedMvBuf);
    assert(inspectMv.has_value());
    assert(inspectMv->unifiedOpcode == 0x03);
    assert(inspectMv->packetLength == 24);
    assert(inspectMv->headerSize == 1);

    // Staly pakiet GC::DEAD (14 / 0x0E, 5B)
    uint8_t fixedDeadBuf[] = { Luria::GC::DEAD, 0x01 };
    auto inspectDead = driver.InspectFrame(fixedDeadBuf);
    assert(inspectDead.has_value());
    assert(inspectDead->unifiedOpcode == 0x0E);
    assert(inspectDead->packetLength == 5);
    assert(inspectDead->headerSize == 1);

    // Staly pakiet GC::TARGET_HP (0x6C, 13B)
    uint8_t fixedHpBuf[] = { Luria::GC::TARGET_HP, 0x01 };
    auto inspectHp = driver.InspectFrame(fixedHpBuf);
    assert(inspectHp.has_value());
    assert(inspectHp->unifiedOpcode == 0x6C);
    assert(inspectHp->packetLength == 13);
    assert(inspectHp->headerSize == 1);

    // Staly pakiet GC::DAMAGE_INFO (135 / 0x87, 10B)
    uint8_t fixedDmgBuf[] = { Luria::GC::DAMAGE_INFO, 0x01 };
    auto inspectDmg = driver.InspectFrame(fixedDmgBuf);
    assert(inspectDmg.has_value());
    assert(inspectDmg->unifiedOpcode == 0x87);
    assert(inspectDmg->packetLength == 10);
    assert(inspectDmg->headerSize == 1);

    // 2. InspectFrame - dynamiczne pakiety
    // 0xBE (32-bit length)
    // < 5B
    uint8_t short32Buf[] = { 0xBE, 0x20, 0x00 };
    assert(!driver.InspectFrame(short32Buf).has_value());

    // >= 5B (length 100)
    uint8_t valid32Buf[10] = { 0xBE, 100, 0, 0, 0 };
    auto inspect32 = driver.InspectFrame(valid32Buf);
    assert(inspect32.has_value());
    assert(inspect32->unifiedOpcode == 0xBE);
    assert(inspect32->packetLength == 100);
    assert(inspect32->headerSize == 1);

    // Dynamiczny 16-bit GC::CHAT (0x04)
    // < 3B
    uint8_t shortChatBuf[] = { 0x04, 0x20 };
    assert(!driver.InspectFrame(shortChatBuf).has_value());

    // >= 3B (length 45)
    uint8_t validChatBuf[] = { 0x04, 45, 0, 0 };
    auto inspectChat = driver.InspectFrame(validChatBuf);
    assert(inspectChat.has_value());
    assert(inspectChat->unifiedOpcode == 0x04);
    assert(inspectChat->packetLength == 45);
    assert(inspectChat->headerSize == 1);

    // Dynamiczny 16-bit GC::WHISPER (0x05, length 20)
    uint8_t validWhisperBuf[] = { 0x05, 20, 0, 0 };
    auto inspectWhisper = driver.InspectFrame(validWhisperBuf);
    assert(inspectWhisper.has_value());
    assert(inspectWhisper->unifiedOpcode == 0x05);
    assert(inspectWhisper->packetLength == 20);
    assert(inspectWhisper->headerSize == 1);

    // 3. EncodeAttack - 8B TPacketCGAttackLuria z rotujaca CRC_XOR_TABLE
    Network::Domain::AttackCommand atkCmd{
        .targetVid = 12345,
        .attackerVid = 1,
        .attackType = 2,
        .sequence = 3,
        .attackMotion = 0
    };
    auto encodedAtk = driver.EncodeAttack(atkCmd);
    assert(encodedAtk.has_value());
    assert(encodedAtk->size() == sizeof(Luria::TPacketCGAttackLuria));
    assert(encodedAtk->size() == 8);
    assert((*encodedAtk)[0] == Luria::CG::ATTACK); // 0x02
    assert((*encodedAtk)[1] == 2); // bType
    uint32_t encVictimVid = 0;
    std::memcpy(&encVictimVid, encodedAtk->data() + 2, sizeof(uint32_t));
    assert(encVictimVid == 12345);
    assert((*encodedAtk)[6] == Luria::CRC_XOR_TABLE[(3 * 2) % 8]);
    assert((*encodedAtk)[7] == Luria::CRC_XOR_TABLE[(3 * 2 + 1) % 8]);

    // 4. EncodeMove - 19B TPacketCGMoveLuria z float fRot
    Network::Domain::MoveCommand mvCmd{
        .vid = 1,
        .x = 25000,
        .y = 35000,
        .rotationDegrees = 135.5f,
        .time = 9999,
        .func = 1,
        .arg = 0
    };
    auto encodedMv = driver.EncodeMove(mvCmd);
    assert(encodedMv.has_value());
    assert(encodedMv->size() == sizeof(Luria::TPacketCGMoveLuria));
    assert(encodedMv->size() == 19);
    assert((*encodedMv)[0] == Luria::CG::MOVE); // 0x03
    assert((*encodedMv)[1] == 1); // bFunc
    assert((*encodedMv)[2] == 0); // bArg
    float encRot = 0.0f;
    int32_t encX = 0, encY = 0;
    uint32_t encTime = 0;
    std::memcpy(&encRot, encodedMv->data() + 3, sizeof(float));
    std::memcpy(&encX, encodedMv->data() + 7, sizeof(int32_t));
    std::memcpy(&encY, encodedMv->data() + 11, sizeof(int32_t));
    std::memcpy(&encTime, encodedMv->data() + 15, sizeof(uint32_t));
    assert(std::fabs(encRot - 135.5f) < 0.001f);
    assert(encX == 25000);
    assert(encY == 35000);
    assert(encTime == 9999);

    // 5. EncodeItemUse - 4B TPacketCGItemUseLuria
    Network::Domain::ItemUseCommand itemCmd{
        .window = 1,
        .cell = 42
    };
    auto encodedItem = driver.EncodeItemUse(itemCmd);
    assert(encodedItem.has_value());
    assert(encodedItem->size() == sizeof(Luria::TPacketCGItemUseLuria));
    assert(encodedItem->size() == 4);
    assert((*encodedItem)[0] == Luria::CG::ITEM_USE); // 0x0B
    assert((*encodedItem)[1] == 1); // window_type
    uint16_t encCell = 0;
    std::memcpy(&encCell, encodedItem->data() + 2, sizeof(uint16_t));
    assert(encCell == 42);

    // 6. EncodeTarget - 5B TPacketCGTargetLuria
    Network::Domain::TargetCommand tgtCmd{
        .targetVid = 555666
    };
    auto encodedTarget = driver.EncodeTarget(tgtCmd);
    assert(encodedTarget.has_value());
    assert(encodedTarget->size() == sizeof(Luria::TPacketCGTargetLuria));
    assert(encodedTarget->size() == 5);
    assert((*encodedTarget)[0] == Luria::CG::TARGET); // 0x3D
    uint32_t encTargetVid = 0;
    std::memcpy(&encTargetVid, encodedTarget->data() + 1, sizeof(uint32_t));
    assert(encTargetVid == 555666);

    // 7. EncodeItemMove - 9B TPacketCGItemMoveLuria (0x0D)
    Network::Domain::ItemMoveCommand moveCmd{
        .srcWindow = 1,
        .srcCell = 10,
        .dstWindow = 1,
        .dstCell = 20,
        .count = 5
    };
    auto encodedItemMove = driver.EncodeItemMove(moveCmd);
    assert(encodedItemMove.has_value());
    assert(encodedItemMove->size() == sizeof(Luria::TPacketCGItemMoveLuria));
    assert(encodedItemMove->size() == 9);
    assert((*encodedItemMove)[0] == Luria::CG::ITEM_MOVE); // 0x0D
    assert((*encodedItemMove)[1] == 1); // srcWindow
    uint16_t encSrcCell = 0;
    std::memcpy(&encSrcCell, encodedItemMove->data() + 2, sizeof(uint16_t));
    assert(encSrcCell == 10);
    assert((*encodedItemMove)[4] == 1); // dstWindow
    uint16_t encDstCell = 0;
    std::memcpy(&encDstCell, encodedItemMove->data() + 5, sizeof(uint16_t));
    assert(encDstCell == 20);
    uint16_t encCount = 0;
    std::memcpy(&encCount, encodedItemMove->data() + 7, sizeof(uint16_t));
    assert(encCount == 5);

    // 8. EncodeItemDrop - 8B TPacketCGItemDropLuria (0x0C)
    Network::Domain::ItemDropCommand dropCmd{
        .window = 1,
        .cell = 15,
        .count = 100
    };
    auto encodedItemDrop = driver.EncodeItemDrop(dropCmd);
    assert(encodedItemDrop.has_value());
    assert(encodedItemDrop->size() == sizeof(Luria::TPacketCGItemDropLuria));
    assert(encodedItemDrop->size() == 8);
    assert((*encodedItemDrop)[0] == Luria::CG::ITEM_DROP); // 0x0C
    assert((*encodedItemDrop)[1] == 1);
    uint16_t encDropCell = 0;
    std::memcpy(&encDropCell, encodedItemDrop->data() + 2, sizeof(uint16_t));
    assert(encDropCell == 15);
    uint32_t encDropCount = 0;
    std::memcpy(&encDropCount, encodedItemDrop->data() + 4, sizeof(uint32_t));
    assert(encDropCount == 100);

    // 9. EncodeItemPickUp - 5B TPacketCGItemPickUpLuria (0x0F)
    Network::Domain::ItemPickUpCommand pickCmd{
        .vid = 778899
    };
    auto encodedPickUp = driver.EncodeItemPickUp(pickCmd);
    assert(encodedPickUp.has_value());
    assert(encodedPickUp->size() == sizeof(Luria::TPacketCGItemPickUpLuria));
    assert(encodedPickUp->size() == 5);
    assert((*encodedPickUp)[0] == Luria::CG::ITEM_PICKUP); // 0x0F
    uint32_t encPickVid = 0;
    std::memcpy(&encPickVid, encodedPickUp->data() + 1, sizeof(uint32_t));
    assert(encPickVid == 778899);

    // Test przeciazenia po uint32_t VID
    auto encodedPickUpVid = driver.EncodeItemPickUp(778899);
    assert(encodedPickUpVid.has_value());
    assert(encodedPickUpVid->size() == 5);
    assert((*encodedPickUpVid)[0] == Luria::CG::ITEM_PICKUP);

    // 10. EncodeSelectEmpire - 2B TPacketCGEmpireLuria (0x5A)
    Network::Domain::EmpireSelectCommand empCmd{
        .empireId = 2
    };
    auto encodedEmpire = driver.EncodeSelectEmpire(empCmd);
    assert(encodedEmpire.has_value());
    assert(encodedEmpire->size() == sizeof(Luria::TPacketCGEmpireLuria));
    assert(encodedEmpire->size() == 2);
    assert((*encodedEmpire)[0] == Luria::CG::EMPIRE); // 0x5A
    assert((*encodedEmpire)[1] == 2);

    // 11. EncodeSelectCharacter - 2B TPacketCGSelectCharacterLuria (0x06)
    Network::Domain::CharacterSelectCommand selCharCmd{
        .slotIndex = 3
    };
    auto encodedSelChar = driver.EncodeSelectCharacter(selCharCmd);
    assert(encodedSelChar.has_value());
    assert(encodedSelChar->size() == sizeof(Luria::TPacketCGSelectCharacterLuria));
    assert(encodedSelChar->size() == 2);
    assert((*encodedSelChar)[0] == Luria::CG::CHARACTER_SELECT); // 0x06
    assert((*encodedSelChar)[1] == 3);

    // 12. EncodeCreateCharacter - 34B TPacketCGCreateCharacterLuria (0x04)
    Network::Domain::CharacterCreateCommand createCharCmd{
        .slotIndex = 0,
        .name = "WarriorPL",
        .job = 1,
        .shape = 2,
        .con = 10,
        .int_ = 5,
        .str = 20,
        .dex = 8
    };
    auto encodedCreateChar = driver.EncodeCreateCharacter(createCharCmd);
    assert(encodedCreateChar.has_value());
    assert(encodedCreateChar->size() == sizeof(Luria::TPacketCGCreateCharacterLuria));
    assert(encodedCreateChar->size() == 34);
    assert((*encodedCreateChar)[0] == Luria::CG::CHARACTER_CREATE); // 0x04
    assert((*encodedCreateChar)[1] == 0); // slotIndex
    assert(std::strcmp(reinterpret_cast<const char*>(encodedCreateChar->data() + 2), "WarriorPL") == 0);
    uint16_t encJob = 0;
    std::memcpy(&encJob, encodedCreateChar->data() + 27, sizeof(uint16_t));
    assert(encJob == 1);
    assert((*encodedCreateChar)[29] == 2);  // shape
    assert((*encodedCreateChar)[30] == 10); // con
    assert((*encodedCreateChar)[31] == 5);  // int_
    assert((*encodedCreateChar)[32] == 20); // str
    assert((*encodedCreateChar)[33] == 8);  // dex

    // 13. EncodeDestroyCharacter - 10B TPacketCGDestroyCharacterLuria (0x05)
    Network::Domain::CharacterDestroyCommand delCharCmd{
        .slotIndex = 1,
        .privateCode = "1234567"
    };
    auto encodedDelChar = driver.EncodeDestroyCharacter(delCharCmd);
    assert(encodedDelChar.has_value());
    assert(encodedDelChar->size() == sizeof(Luria::TPacketCGDestroyCharacterLuria));
    assert(encodedDelChar->size() == 10);
    assert((*encodedDelChar)[0] == Luria::CG::CHARACTER_DELETE); // 0x05
    assert((*encodedDelChar)[1] == 1);
    assert(std::strcmp(reinterpret_cast<const char*>(encodedDelChar->data() + 2), "1234567") == 0);

    // 14. EncodeEnterGame - 68B TPacketCGClientVersionLuria + Luria::CG::ENTERGAME
    auto encodedEnterGame = driver.EncodeEnterGame();
    assert(encodedEnterGame.has_value());
    assert(encodedEnterGame->size() == sizeof(Luria::TPacketCGClientVersionLuria) + 1);
    assert(encodedEnterGame->size() == 68);
    assert((*encodedEnterGame)[0] == Luria::CG::CLIENT_VERSION); // 0xF1
    assert(std::strcmp(reinterpret_cast<const char*>(encodedEnterGame->data() + 1), "metin2client.exe") == 0);
    assert(std::strcmp(reinterpret_cast<const char*>(encodedEnterGame->data() + 34), "1215955205") == 0);
    assert((*encodedEnterGame)[67] == Luria::CG::ENTERGAME); // 0x0A

    // 15. DispatchInbound - mapowanie na MockGameEventSink
    MockGameEventSink mockSink;

    // Dispatch MOVE (0x03)
    Luria::TPacketGCMoveLuria gcMv{};
    gcMv.header = Luria::GC::CHARACTER_MOVE;
    gcMv.bFunc = 1;
    gcMv.bArg = 0;
    gcMv.bRot = 36; // 180 stopni
    gcMv.dwVID = 1111;
    gcMv.lX = 5000;
    gcMv.lY = 6000;
    gcMv.dwTime = 12345;
    gcMv.dwDuration = 100;
    std::span<const uint8_t> mvSpan(reinterpret_cast<const uint8_t*>(&gcMv), sizeof(gcMv));

    bool mvRes = driver.DispatchInbound(0x03, mvSpan, &mockSink);
    assert(mvRes);
    assert(mockSink.actorMovedCalled);
    assert(mockSink.lastMovedVid == 1111);
    assert(mockSink.lastMovedX == 5000);
    assert(mockSink.lastMovedY == 6000);
    assert(std::fabs(mockSink.lastMovedRot - 180.0f) < 0.001f);

    // Dispatch DEAD (0x0E)
    mockSink.Reset();
    Luria::TPacketGCDeadLuria gcDead{};
    gcDead.header = Luria::GC::DEAD;
    gcDead.dwVID = 2222;
    std::span<const uint8_t> deadSpan(reinterpret_cast<const uint8_t*>(&gcDead), sizeof(gcDead));

    bool deadRes = driver.DispatchInbound(0x0E, deadSpan, &mockSink);
    assert(deadRes);
    assert(mockSink.actorDeadCalled);
    assert(mockSink.lastDeadVid == 2222);

    // Dispatch TARGET_HP (0x6C)
    mockSink.Reset();
    Luria::TPacketGCTargetHPLuria gcHp{};
    gcHp.header = Luria::GC::TARGET_HP;
    gcHp.dwTargetVID = 3333;
    gcHp.llCurrentHP = 50000;
    std::span<const uint8_t> hpSpan(reinterpret_cast<const uint8_t*>(&gcHp), sizeof(gcHp));

    bool hpRes = driver.DispatchInbound(0x6C, hpSpan, &mockSink);
    assert(hpRes);
    assert(mockSink.targetChangedCalled);
    assert(mockSink.lastTargetVid == 3333);

    // Dispatch DAMAGE_INFO (0x87)
    mockSink.Reset();
    Luria::TPacketGCDamageInfoLuria gcDmg{};
    gcDmg.header = Luria::GC::DAMAGE_INFO;
    gcDmg.dwVID = 4444;
    gcDmg.flag = 2;
    gcDmg.damage = 9876;
    std::span<const uint8_t> dmgSpan(reinterpret_cast<const uint8_t*>(&gcDmg), sizeof(gcDmg));

    bool dmgRes = driver.DispatchInbound(0x87, dmgSpan, &mockSink);
    assert(dmgRes);
    assert(mockSink.damageInfoCalled);
    assert(mockSink.lastDamageVictimVid == 4444);
    assert(mockSink.lastDamageAmount == 9876);
    assert(mockSink.lastDamageFlag == 2);

    // Dispatch pustego bufora zwraca false
    assert(!driver.DispatchInbound(0x03, {}, &mockSink));

    std::cout << "[PASS] LuriaProtocolDriver dziala prawidlowo!" << std::endl;
}

int main()
{
    std::cout << "==========================================================" << std::endl;
    std::cout << " TEST C34: UNIVERSAL PROTOCOL DRIVERS (FILAR C MASTER PLAN) " << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestProtocolDriverRegistry();
    TestStandardX64Driver();
    TestBeaviumDriver();
    TestMartySamaDriver();
    TestPolymorphicDrivers();
    TestLuriaProtocolDefinition();
    TestLuriaDriver();

    std::cout << "==========================================================" << std::endl;
    std::cout << " WSZYSTKIE TESTY STEROWNIKOW PROTOKOLU ZAKONCZONE SUKCESEM " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
