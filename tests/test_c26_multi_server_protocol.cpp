#include <cassert>
#include <iostream>
#include <vector>
#include <string_view>
#include <cmath>

#include "Client/Network/Domain/CombatCommands.h"
#include "Client/Network/Domain/MovementCommands.h"
#include "Client/Network/Protocol/IServerProtocolDriver.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Protocol/Drivers/StandardX64ProtocolDriver.h"
#include "Client/Network/Protocol/Drivers/BeaviumProtocolDriver.h"
#include "Client/Network/Protocol/Protocol.h"
#include "Client/Network/Protocol/BeaviumProtocol.h"
#include "UserInterface/Contracts/IGameEvents.h"

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
};

int main()
{
    std::cout << "[UPM Test] Rozpoczynam testy Uniwersalnej Macierzy Protokolow (Krok 3 i Krok 4)..." << std::endl;

    // ========================================================================
    // 1. Testy StandardX64ProtocolDriver
    // ========================================================================
    {
        std::cout << "[Test 1] Testowanie StandardX64ProtocolDriver..." << std::endl;
        Network::Protocol::Drivers::StandardX64ProtocolDriver driver;

        assert(driver.GetDriverName() == "standard_x64");

        // InspectFrame - za krotki bufor (< 4B)
        uint8_t shortBuf[] = { 0x01, 0x04, 0x0B };
        auto inspectShort = driver.InspectFrame(shortBuf);
        assert(!inspectShort.has_value());

        // InspectFrame - niepoprawna dlugosc (< 4B)
        uint8_t badLenBuf[] = { 0x01, 0x04, 0x02, 0x00 }; // len = 2 (< 4)
        auto inspectBadLen = driver.InspectFrame(badLenBuf);
        assert(!inspectBadLen.has_value());

        // InspectFrame - niepoprawna dlugosc (> 65000B)
        uint8_t hugeLenBuf[] = { 0x01, 0x04, 0xFF, 0xFF }; // len = 65535 (> 65000)
        auto inspectHugeLen = driver.InspectFrame(hugeLenBuf);
        assert(!inspectHugeLen.has_value());

        // InspectFrame - poprawna ramka 4B: opcode 0x0401, len 11
        uint8_t validFrameBuf[] = { 0x01, 0x04, 0x0B, 0x00, 0x00, 0x00, 0x00 };
        auto inspectValid = driver.InspectFrame(validFrameBuf);
        assert(inspectValid.has_value());
        assert(inspectValid->unifiedOpcode == 0x0401);
        assert(inspectValid->packetLength == 11);
        assert(inspectValid->headerSize == 4);

        // EncodeAttack - serializacja do 11 bajtow
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

        const auto* pAtkPacket = reinterpret_cast<const TPacketCGAttack*>(encodedAtk->data());
        assert(pAtkPacket->header == 0x0401);
        assert(pAtkPacket->length == 11);
        assert(pAtkPacket->bType == 2);
        assert(pAtkPacket->dwVictimVID == 12345);

        // EncodeMove - serializacja do 19 bajtow
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

        const auto* pMvPacket = reinterpret_cast<const TPacketCGMove*>(encodedMv->data());
        assert(pMvPacket->header == 0x0301);
        assert(pMvPacket->length == 19);
        assert(pMvPacket->bFunc == 1);
        assert(pMvPacket->bArg == 2);
        assert(pMvPacket->bRot == static_cast<uint8_t>(90.0f / 5.0f)); // 18
        assert(pMvPacket->lX == 50000);
        assert(pMvPacket->lY == 60000);
        assert(pMvPacket->dwTime == 1000);

        // DispatchInbound - pusty bufor
        assert(!driver.DispatchInbound(0x0401, {}, nullptr));
    }

    // ========================================================================
    // 2. Testy BeaviumProtocolDriver
    // ========================================================================
    {
        std::cout << "[Test 2] Testowanie BeaviumProtocolDriver..." << std::endl;
        Network::Protocol::Drivers::BeaviumProtocolDriver driver;

        assert(driver.GetDriverName() == "beavium");

        // InspectFrame - pusty bufor
        assert(!driver.InspectFrame({}).has_value());

        // InspectFrame - nieznany opkod (np. 0xFE nie ma w beavium_gc_table.inl)
        uint8_t unknownBuf[] = { 0xFE, 0x00, 0x00 };
        assert(!driver.InspectFrame(unknownBuf).has_value());

        // InspectFrame - pakiet o stalym rozmiarze GC::ATTACK (0x0C, rozmiar 12B)
        uint8_t fixedAtkBuf[] = { Beavium::GC::ATTACK, 0x01, 0x02 };
        auto inspectFixedAtk = driver.InspectFrame(fixedAtkBuf);
        assert(inspectFixedAtk.has_value());
        assert(inspectFixedAtk->unifiedOpcode == 0x0C);
        assert(inspectFixedAtk->packetLength == 12);
        assert(inspectFixedAtk->headerSize == 1);

        // InspectFrame - pakiet o stalym rozmiarze GC::DEAD (0x0E, rozmiar 26B)
        uint8_t fixedDeadBuf[] = { Beavium::GC::DEAD };
        auto inspectFixedDead = driver.InspectFrame(fixedDeadBuf);
        assert(inspectFixedDead.has_value());
        assert(inspectFixedDead->unifiedOpcode == 0x0E);
        assert(inspectFixedDead->packetLength == 26);
        assert(inspectFixedDead->headerSize == 1);

        // InspectFrame - pakiet dynamiczny GC::CHAT (0x04)
        // Bufor za krotki (< 3B dla pakietu dynamicznego)
        uint8_t shortDynBuf[] = { Beavium::GC::CHAT, 0x10 };
        assert(!driver.InspectFrame(shortDynBuf).has_value());

        // Bufor dynamiczny: header=0x04, size=32 (0x0020)
        uint8_t validDynBuf[] = { Beavium::GC::CHAT, 0x20, 0x00, 0xFF, 0xEE };
        auto inspectDyn = driver.InspectFrame(validDynBuf);
        assert(inspectDyn.has_value());
        assert(inspectDyn->unifiedOpcode == 0x04);
        assert(inspectDyn->packetLength == 32);
        assert(inspectDyn->headerSize == 1);

        // EncodeAttack - ProxyPacketCGAttack (8 bajtow, naglowek 0x02)
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
        assert((*encodedAtk)[0] == Beavium::CG::ATTACK); // 0x02
        assert((*encodedAtk)[1] == 0); // attackType

        uint32_t encTargetVid = *reinterpret_cast<const uint32_t*>(encodedAtk->data() + 2);
        uint16_t encSeq = *reinterpret_cast<const uint16_t*>(encodedAtk->data() + 6);
        assert(encTargetVid == 77777);
        assert(encSeq == 1234);

        // EncodeMove - Beavium::TPacketCGMoveBeavium (24 bajty, naglowek 0x07, mikrostopnie)
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
        assert(encodedMv->size() == 24);

        const auto* pBeavMv = reinterpret_cast<const Beavium::TPacketCGMoveBeavium*>(encodedMv->data());
        assert(pBeavMv->header == Beavium::CG::MOVE); // 0x07
        assert(pBeavMv->bFunc == 1);
        assert(pBeavMv->wArg == 15);
        assert(pBeavMv->dwRot == 180000000); // 180 * 1e6 mikrostopni
        assert(pBeavMv->lX == 100000);
        assert(pBeavMv->lY == 200000);
        assert(pBeavMv->dwTime == 5000);

        // DispatchInbound - testowanie mapowania zdarzen na MockGameEventSink
        MockGameEventSink mockSink;

        // Dispatch GC::ATTACK (0x0C)
        Beavium::TPacketGCAttackBeavium gcAtk{};
        gcAtk.header = Beavium::GC::ATTACK;
        gcAtk.dwVID = 55;
        gcAtk.dwVictimVID = 99;
        gcAtk.bType = 3;
        gcAtk.wMotionArg = 0;
        std::span<const uint8_t> atkSpan(reinterpret_cast<const uint8_t*>(&gcAtk), sizeof(gcAtk));

        bool dispatchAtkRes = driver.DispatchInbound(Beavium::GC::ATTACK, atkSpan, &mockSink);
        assert(dispatchAtkRes);
        assert(mockSink.attackExecutedCalled);
        assert(mockSink.lastAttackerVid == 55);
        assert(mockSink.lastVictimVid == 99);
        assert(mockSink.lastMotionType == 3);

        // Dispatch GC::DEAD (0x0E)
        Beavium::TPacketGCDeadBeavium gcDead{};
        gcDead.header = Beavium::GC::DEAD;
        gcDead.dwVID = 444;
        std::span<const uint8_t> deadSpan(reinterpret_cast<const uint8_t*>(&gcDead), sizeof(gcDead));

        bool dispatchDeadRes = driver.DispatchInbound(Beavium::GC::DEAD, deadSpan, &mockSink);
        assert(dispatchDeadRes);
        assert(mockSink.actorDeadCalled);
        assert(mockSink.lastDeadVid == 444);

        // Dispatch GC::CHARACTER_MOVE (0x03)
        Beavium::TPacketGCCharacterMoveBeavium gcMv{};
        gcMv.header = Beavium::GC::CHARACTER_MOVE;
        gcMv.dwVID = 888;
        gcMv.lX = 1234;
        gcMv.lY = 5678;
        gcMv.dwRot = 90000000; // 90 stopni
        gcMv.dwTime = 9999;
        std::span<const uint8_t> mvSpan(reinterpret_cast<const uint8_t*>(&gcMv), sizeof(gcMv));

        bool dispatchMvRes = driver.DispatchInbound(Beavium::GC::CHARACTER_MOVE, mvSpan, &mockSink);
        assert(dispatchMvRes);
        assert(mockSink.actorMovedCalled);
        assert(mockSink.lastMovedVid == 888);
        assert(mockSink.lastMovedX == 1234);
        assert(mockSink.lastMovedY == 5678);
        assert(std::fabs(mockSink.lastMovedRot - 90.0f) < 0.001f);
    }

    // ========================================================================
    // 3. Testy ProtocolDriverRegistry (Singleton, Rejestracja, Przelaczanie)
    // ========================================================================
    {
        std::cout << "[Test 3] Testowanie ProtocolDriverRegistry..." << std::endl;
        auto& registry = Network::Protocol::ProtocolDriverRegistry::Instance();
        registry.Clear();

        assert(registry.GetActiveDriver() == nullptr);
        assert(!registry.HasDriver("standard_x64"));
        assert(!registry.HasDriver("beavium"));

        // InitializeDefaults
        registry.InitializeDefaults();
        assert(registry.HasDriver("standard_x64"));
        assert(registry.HasDriver("beavium"));

        // Domyslny aktywny
        auto* activeDriver = registry.GetActiveDriver();
        assert(activeDriver != nullptr);
        assert(activeDriver->GetDriverName() == "standard_x64");
        assert(registry.GetActiveDriverName() == "standard_x64");

        // Dynamiczne przelaczenie w runtime na Beavium
        bool switchedToBeavium = registry.SetActiveDriver("beavium");
        assert(switchedToBeavium);
        assert(registry.GetActiveDriver()->GetDriverName() == "beavium");
        assert(registry.GetActiveDriverName() == "beavium");

        // Dynamiczne przelaczenie z powrotem na StandardX64
        bool switchedToX64 = registry.SetActiveDriver("standard_x64");
        assert(switchedToX64);
        assert(registry.GetActiveDriver()->GetDriverName() == "standard_x64");
        assert(registry.GetActiveDriverName() == "standard_x64");

        // Proba aktywacji nieznanego sterownika
        bool switchedUnknown = registry.SetActiveDriver("non_existent_server");
        assert(!switchedUnknown);
        assert(registry.GetActiveDriver()->GetDriverName() == "standard_x64");

        // Pobranie sterownika bez zmiany aktywnego
        auto* pBeavDriver = registry.GetDriver("beavium");
        assert(pBeavDriver != nullptr);
        assert(pBeavDriver->GetDriverName() == "beavium");
        assert(registry.GetActiveDriver()->GetDriverName() == "standard_x64");
    }

    std::cout << "test_c26_multi_server_protocol: ALL TESTS PASSED (100% PASS)" << std::endl;
    return 0;
}
