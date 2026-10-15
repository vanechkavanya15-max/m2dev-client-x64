#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <cstddef>

#include "Client/Bridge/BotApi_FFI.h"
#include "Client/Contracts/IPlayerDomainContract.h"
#include "Client/Contracts/IInventoryDomainContract.h"
#include "Client/Contracts/ICombatDomainContract.h"
#include "Client/Contracts/IWorldQueryContract.h"
#include "Client/MCP/AgentMCPBridge.h"

using namespace Client::Contracts;
using namespace Client::MCP;

// ============================================================================
// MOCK IMPLEMENTACJE DLA TESTOW JEDNOSTKOWYCH C-ABI
// ============================================================================

class MockPlayerForBot : public IPlayerDomainContract
{
public:
    uint32_t GetPlayerVID() const noexcept override { return 10001; }
    std::string_view GetPlayerName() const noexcept override { return "BotMaster"; }
    int GetPlayerLevel() const noexcept override { return 90; }

    int64_t GetCurrentHP() const noexcept override { return 18500; }
    int64_t GetMaxHP() const noexcept override { return 20000; }
    int64_t GetCurrentSP() const noexcept override { return 4500; }
    int64_t GetMaxSP() const noexcept override { return 5000; }
    int64_t GetYang() const noexcept override { return 99999999; }
    int64_t GetExp() const noexcept override { return 500000; }

    PlayerCoordinates GetCoordinates() const noexcept override { return {1000.0f, 2000.0f, 50.0f, 180.0f}; }
    uint32_t GetTargetVID() const noexcept override { return 6001; }
    bool IsInCombat() const noexcept override { return true; }

    PlayerStatusSnapshot CaptureSnapshot() const override
    {
        return PlayerStatusSnapshot{
            .vid = 10001,
            .name = "BotMaster",
            .level = 90,
            .hp = 18500,
            .maxHp = 20000,
            .sp = 4500,
            .maxSp = 5000,
            .yang = 99999999,
            .exp = 500000,
            .coords = {1000.0f, 2000.0f, 50.0f, 180.0f},
            .targetVid = 6001,
            .isCombatActive = true,
            .mapName = "metin2_map_desert"
        };
    }

    bool RequestMove(float x, float y) override
    {
        m_lastMoveX = x;
        m_lastMoveY = y;
        m_moveCount++;
        return true;
    }

    float m_lastMoveX{0.0f};
    float m_lastMoveY{0.0f};
    int m_moveCount{0};
};

class MockWorldForBot : public IWorldQueryContract
{
public:
    std::vector<EntitySnapshot> GetEntitiesInRadius(float radius) const override
    {
        std::vector<EntitySnapshot> result;

        // Mob (type 2 w EntityType -> 0 w ActorSnapshot)
        result.push_back(EntitySnapshot{
            .vid = 2001,
            .vnum = 101,
            .type = EntityType::Monster,
            .name = "Dziki Pies",
            .x = 1050.0f,
            .y = 2020.0f,
            .z = 50.0f,
            .distance = 53.8f,
            .currentHpPercent = 80,
            .isHostile = true,
            .isDead = false
        });

        // NPC (type 3 w EntityType -> 1 w ActorSnapshot)
        result.push_back(EntitySnapshot{
            .vid = 2002,
            .vnum = 9003,
            .type = EntityType::NPC,
            .name = "Handlarka Roznosci",
            .x = 1020.0f,
            .y = 1990.0f,
            .z = 50.0f,
            .distance = 22.3f,
            .currentHpPercent = 100,
            .isHostile = false,
            .isDead = false
        });

        // Metin / Stone (type 7 w EntityType -> 2 w ActorSnapshot)
        result.push_back(EntitySnapshot{
            .vid = 2003,
            .vnum = 8001,
            .type = EntityType::Stone,
            .name = "Kamien Metin Bitwy",
            .x = 1100.0f,
            .y = 2100.0f,
            .z = 50.0f,
            .distance = 141.4f,
            .currentHpPercent = 45,
            .isHostile = true,
            .isDead = false
        });

        // Inny gracz (type 1 w EntityType -> 6 w ActorSnapshot)
        result.push_back(EntitySnapshot{
            .vid = 2004,
            .vnum = 0,
            .type = EntityType::Player,
            .name = "InnyWojownik",
            .x = 980.0f,
            .y = 2010.0f,
            .z = 50.0f,
            .distance = 22.3f,
            .currentHpPercent = 95,
            .isHostile = false,
            .isDead = false
        });

        // Przedmiot na ziemi (type 6 w EntityType -> GroundItem)
        result.push_back(EntitySnapshot{
            .vid = 9001,
            .vnum = 299,
            .type = EntityType::ItemGround,
            .name = "Miecz Pelni Ksiezyca+9",
            .x = 1010.0f,
            .y = 2005.0f,
            .z = 50.0f,
            .distance = 11.1f,
            .currentHpPercent = 100,
            .isHostile = false,
            .isDead = false
        });

        // Filtruj wedlug podanego promienia
        std::vector<EntitySnapshot> filtered;
        for (const auto& e : result)
        {
            if (e.distance <= radius)
            {
                filtered.push_back(e);
            }
        }
        return filtered;
    }

    std::optional<EntitySnapshot> GetEntityByVID(uint32_t vid) const override
    {
        auto all = GetEntitiesInRadius(10000.0f);
        for (const auto& e : all)
        {
            if (e.vid == vid)
                return e;
        }
        return std::nullopt;
    }

    std::optional<EntitySnapshot> GetNearestHostile() const override { return GetEntityByVID(2001); }
    std::optional<EntitySnapshot> GetNearestItemGround() const override { return GetEntityByVID(9001); }
};

class MockCombatForBot : public ICombatDomainContract
{
public:
    CombatVerdict EvaluateAttack(uint32_t targetVid) const override
    {
        CombatVerdict verdict{};
        if (targetVid == 0)
        {
            verdict.canAttack = false;
            verdict.reason = "INVALID_TARGET_VID";
            return verdict;
        }
        if (targetVid == 9999) // Cel nietykalny
        {
            verdict.canAttack = false;
            verdict.reason = "TARGET_IMMUNE";
            return verdict;
        }
        verdict.canAttack = true;
        verdict.distanceToTarget = 15.0f;
        verdict.reason = "OK";
        return verdict;
    }

    float GetAttackCooldown() const noexcept override { return 0.0f; }
    uint32_t GetLastTargetVID() const noexcept override { return m_lastAttackedVid; }

    bool ExecuteAttack(uint32_t targetVid) override
    {
        if (targetVid == 0 || targetVid == 9999)
            return false;
        m_lastAttackedVid = targetVid;
        m_attackCount++;
        return true;
    }

    void UpdateCombatState(float /*deltaTime*/) override {}

    uint32_t m_lastAttackedVid{0};
    int m_attackCount{0};
};

class MockInventoryForBot : public IInventoryDomainContract
{
public:
    std::optional<ItemDataSnapshot> GetItem(const ItemCellAddress& pos) const override
    {
        if (pos.cellIndex == 0)
        {
            return ItemDataSnapshot{
                .pos = pos,
                .vnum = 27003,
                .count = 200,
                .localizedName = "Czerwona Mikstura(D)"
            };
        }
        return std::nullopt;
    }

    std::vector<ItemDataSnapshot> GetAllItems(uint8_t windowType) const override
    {
        std::vector<ItemDataSnapshot> list;
        list.push_back(ItemDataSnapshot{
            .pos = {windowType, 0},
            .vnum = 27003,
            .count = 200,
            .localizedName = "Czerwona Mikstura(D)"
        });
        return list;
    }

    bool IsCellEmpty(const ItemCellAddress& pos) const noexcept override { return pos.cellIndex != 0; }
    size_t GetFreeSlotCount(uint8_t /*windowType*/) const noexcept override { return 44; }

    bool RequestUseItem(const ItemCellAddress& pos) override
    {
        if (pos.cellIndex == 0)
        {
            m_usedCell = pos.cellIndex;
            m_usedWindow = pos.windowType;
            m_useCount++;
            return true;
        }
        return false;
    }

    bool RequestMoveItem(const ItemCellAddress&, const ItemCellAddress&, uint32_t) override { return true; }
    bool RequestDropItem(const ItemCellAddress&, uint32_t) override { return true; }

    uint16_t m_usedCell{0xFFFF};
    uint8_t m_usedWindow{0};
    int m_useCount{0};
};

// ============================================================================
// TESTY JEDNOSTKOWE C-ABI
// ============================================================================

void TestBinaryLayoutAndPacking()
{
    std::cout << "[TEST 1] Weryfikacja binarnego ukladu pamieci (ABI & pragma pack 1)..." << std::endl;

    // Rozmiary struktur
    assert(sizeof(ActorSnapshot) == 32);
    assert(sizeof(GroundItemSnapshot) == 20);

    // Dokladne offsety pol w ActorSnapshot
    assert(offsetof(ActorSnapshot, vid) == 0);
    assert(offsetof(ActorSnapshot, vnum) == 4);
    assert(offsetof(ActorSnapshot, x) == 8);
    assert(offsetof(ActorSnapshot, y) == 12);
    assert(offsetof(ActorSnapshot, z) == 16);
    assert(offsetof(ActorSnapshot, hp) == 20);
    assert(offsetof(ActorSnapshot, max_hp) == 24);
    assert(offsetof(ActorSnapshot, type) == 28);
    assert(offsetof(ActorSnapshot, is_dead) == 29);
    assert(offsetof(ActorSnapshot, _reserved) == 30);

    // Dokladne offsety pol w GroundItemSnapshot
    assert(offsetof(GroundItemSnapshot, vid) == 0);
    assert(offsetof(GroundItemSnapshot, vnum) == 4);
    assert(offsetof(GroundItemSnapshot, x) == 8);
    assert(offsetof(GroundItemSnapshot, y) == 12);
    assert(offsetof(GroundItemSnapshot, count) == 16);

    std::cout << "  -> Uklad pamieci: POPRAWNY (32B dla Actor [wyrownane], 20B dla GroundItem)" << std::endl;
}

void TestPlayerPositionAndStatus()
{
    std::cout << "[TEST 2] Test Bot_GetPlayerPos i Bot_GetPlayerStatus..." << std::endl;

    MockPlayerForBot mockPlayer;
    Bot_InitializeContracts(&mockPlayer, nullptr, nullptr, nullptr);

    // 1. Bezpieczenstwo wskaznikow nullptr
    assert(Bot_GetPlayerPos(nullptr, nullptr, nullptr) == true);

    // 2. Pobranie pozycji
    float x = 0.0f, y = 0.0f, z = 0.0f;
    assert(Bot_GetPlayerPos(&x, &y, &z) == true);
    assert(x == 1000.0f);
    assert(y == 2000.0f);
    assert(z == 50.0f);

    // 3. Pobranie statusu (Metin2 POINT_*)
    assert(Bot_GetPlayerStatus(1) == 90);        // POINT_LEVEL
    assert(Bot_GetPlayerStatus(3) == 500000);    // POINT_EXP
    assert(Bot_GetPlayerStatus(5) == 18500);     // POINT_HP
    assert(Bot_GetPlayerStatus(6) == 20000);     // POINT_MAX_HP
    assert(Bot_GetPlayerStatus(7) == 4500);      // POINT_SP
    assert(Bot_GetPlayerStatus(8) == 5000);      // POINT_MAX_SP
    assert(Bot_GetPlayerStatus(11) == 99999999); // POINT_GOLD
    assert(Bot_GetPlayerStatus(999) == 0);       // Nieznany status zwraca 0

    // 4. Test braku kontraktu
    Bot_ResetContracts();
    assert(Bot_GetPlayerPos(&x, &y, &z) == false);

    std::cout << "  -> Pozycja gracza i statusy POINT_*: POPRAWNE" << std::endl;
}

void TestActorsInRadiusZeroAllocation()
{
    std::cout << "[TEST 3] Test Bot_GetActorsInRadius (Zero-Allocation, bez przeladowania bufora)..." << std::endl;

    MockWorldForBot mockWorld;
    Bot_InitializeContracts(nullptr, &mockWorld, nullptr, nullptr);

    // 1. Obsluga blednych danych wejsciowych
    assert(Bot_GetActorsInRadius(100.0f, nullptr, 10) == 0);
    ActorSnapshot dummy[1];
    assert(Bot_GetActorsInRadius(100.0f, dummy, 0) == 0);
    assert(Bot_GetActorsInRadius(100.0f, dummy, -5) == 0);

    // 2. Pobranie aktorow z malym promieniem (promien 60 zawiera Dzikiego Psa, Handlarke i Wojownika)
    ActorSnapshot actorsBuf[16];
    int count = Bot_GetActorsInRadius(60.0f, actorsBuf, 16);
    assert(count == 3);

    // Weryfikacja typow i wlasciwosci
    bool foundMob = false;
    bool foundNpc = false;
    bool foundPlayer = false;

    for (int i = 0; i < count; ++i)
    {
        const auto& a = actorsBuf[i];
        if (a.vid == 2001)
        {
            assert(a.vnum == 101);
            assert(a.type == 0); // 0 = mob
            assert(a.hp == 80);
            assert(!a.is_dead);
            foundMob = true;
        }
        else if (a.vid == 2002)
        {
            assert(a.vnum == 9003);
            assert(a.type == 1); // 1 = npc
            assert(a.hp == 100);
            foundNpc = true;
        }
        else if (a.vid == 2004)
        {
            assert(a.vnum == 0);
            assert(a.type == 6); // 6 = player
            assert(a.hp == 95);
            foundPlayer = true;
        }
    }
    assert(foundMob && foundNpc && foundPlayer);

    // 3. Pobranie z duzym promieniem (zawiera takze Kamien Metin)
    count = Bot_GetActorsInRadius(200.0f, actorsBuf, 16);
    assert(count == 4);

    bool foundMetin = false;
    for (int i = 0; i < count; ++i)
    {
        if (actorsBuf[i].vid == 2003)
        {
            assert(actorsBuf[i].vnum == 8001);
            assert(actorsBuf[i].type == 2); // 2 = metin
            assert(actorsBuf[i].hp == 45);
            foundMetin = true;
        }
    }
    assert(foundMetin);

    // 4. Test obciecia do maxCapacity (brak wyjscia poza zakres bufora)
    ActorSnapshot smallBuf[2];
    int smallCount = Bot_GetActorsInRadius(200.0f, smallBuf, 2);
    assert(smallCount == 2);

    // 5. Test ujemnego promienia i zera (ochrona przed edge case radSq)
    assert(Bot_GetActorsInRadius(-50.0f, actorsBuf, 16) == 0);
    assert(Bot_GetActorsInRadius(0.0f, actorsBuf, 16) == 0);

    std::cout << "  -> Percepcja aktorow z radius, vnum i pojemnoscia bufora: POPRAWNA" << std::endl;
}

void TestGroundItems()
{
    std::cout << "[TEST 4] Test Bot_GetGroundItems..." << std::endl;

    MockWorldForBot mockWorld;
    Bot_InitializeContracts(nullptr, &mockWorld, nullptr, nullptr);

    // 1. Zabezpieczenie przed nullptr
    assert(Bot_GetGroundItems(50.0f, nullptr, 5) == 0);

    // 2. Pobranie z kontraktu swiata
    GroundItemSnapshot itemBuf[8];
    int count = Bot_GetGroundItems(50.0f, itemBuf, 8);
    assert(count == 1);
    assert(itemBuf[0].vid == 9001);
    assert(itemBuf[0].x == 1010.0f);
    assert(itemBuf[0].y == 2005.0f);
    assert(itemBuf[0].count == 1);

    // 3. Test mockowanych itemow na ziemi
    GroundItemSnapshot mockItems[2] = {
        {101, 27001, 1005.0f, 2005.0f, 5},
        {102, 27002, 1008.0f, 2008.0f, 10}
    };
    Bot_SetMockGroundItems(mockItems, 2);

    MockPlayerForBot mockPlayer;
    Bot_InitializeContracts(&mockPlayer, &mockWorld, nullptr, nullptr);

    count = Bot_GetGroundItems(50.0f, itemBuf, 8);
    assert(count == 2);
    assert(itemBuf[0].vid == 101 && itemBuf[0].vnum == 27001 && itemBuf[0].count == 5);
    assert(itemBuf[1].vid == 102 && itemBuf[1].vnum == 27002 && itemBuf[1].count == 10);

    // Reset mockowanych itemow
    Bot_SetMockGroundItems(nullptr, 0);

    std::cout << "  -> Pobieranie przedmiotow z ziemi: POPRAWNE" << std::endl;
}

void TestActionsMoveAttackUse()
{
    std::cout << "[TEST 5] Test Bot_MoveTo, Bot_AttackTarget, Bot_UseItem..." << std::endl;

    MockPlayerForBot mockPlayer;
    MockCombatForBot mockCombat;
    MockInventoryForBot mockInventory;
    Bot_InitializeContracts(&mockPlayer, nullptr, &mockCombat, &mockInventory);

    // 1. Test Bot_MoveTo
    assert(Bot_MoveTo(1250.0f, 2350.0f));
    assert(mockPlayer.m_moveCount == 1);
    assert(mockPlayer.m_lastMoveX == 1250.0f);
    assert(mockPlayer.m_lastMoveY == 2350.0f);

    // Brak gracza musi zwracac false (weryfikacja usuniecia krytycznego bledu)
    Bot_ResetContracts();
    assert(!Bot_MoveTo(1250.0f, 2350.0f));
    Bot_InitializeContracts(&mockPlayer, nullptr, &mockCombat, &mockInventory);

    // Niepoprawne wspolrzedne (NaN / Inf) musza zostac odrzucone
    assert(!Bot_MoveTo(std::numeric_limits<float>::quiet_NaN(), 100.0f));
    assert(!Bot_MoveTo(100.0f, std::numeric_limits<float>::infinity()));

    // 2. Test Bot_AttackTarget
    assert(!Bot_AttackTarget(0)); // VID = 0 niepoprawny
    assert(!Bot_AttackTarget(9999)); // Cel niewrazliwy
    assert(Bot_AttackTarget(2001)); // Prawidlowy mob
    assert(mockCombat.m_attackCount == 1);
    assert(mockCombat.m_lastAttackedVid == 2001);

    // 3. Test Bot_UseItem
    assert(Bot_UseItem(1, 0)); // Slot 0 z mikstura
    assert(mockInventory.m_useCount == 1);
    assert(mockInventory.m_usedCell == 0);
    assert(mockInventory.m_usedWindow == 1);

    assert(!Bot_UseItem(1, 5)); // Pusty slot zwraca false

    std::cout << "  -> Akcje ruchu, ataku i uzycia przedmiotu: POPRAWNE" << std::endl;
}

void TestWindowVisibilityAndThrottle()
{
    std::cout << "[TEST 6] Test Bot_SetWindowVisibility, Bot_IsWindowVisible, Bot_SetRenderThrottle..." << std::endl;

    // 1. Domyslny stan
    Bot_ResetContracts();
    assert(Bot_IsWindowVisible() == true);
    assert(Bot_IsRenderThrottled() == false);

    // 2. Zmiana widocznosci
    Bot_SetWindowVisibility(false);
    assert(Bot_IsWindowVisible() == false);

    Bot_SetWindowVisibility(true);
    assert(Bot_IsWindowVisible() == true);

    // 3. Dlawienie renderowania
    Bot_SetRenderThrottle(true);
    assert(Bot_IsRenderThrottled() == true);

    Bot_SetRenderThrottle(false);
    assert(Bot_IsRenderThrottled() == false);

    // 4. Zarejestrowanie dedykowanych callbackow kontroli okna
    static bool s_mockWindowVisible = true;
    static bool s_mockThrottle = false;

    Bot_SetWindowControlHandlers(
        [](bool visible) { s_mockWindowVisible = visible; },
        []() -> bool { return s_mockWindowVisible; },
        [](bool throttle) { s_mockThrottle = throttle; }
    );

    Bot_SetWindowVisibility(false);
    assert(!s_mockWindowVisible);
    assert(!Bot_IsWindowVisible());

    Bot_SetRenderThrottle(true);
    assert(s_mockThrottle);
    assert(Bot_IsRenderThrottled());

    // Sprzatanie
    Bot_ResetContracts();

    std::cout << "  -> Kontrola widocznosci okna i dlawienia renderowania: POPRAWNA" << std::endl;
}

void TestFallbackViaAgentMCPBridge()
{
    std::cout << "[TEST 7] Test fallbacku do AgentMCPBridge gdy brak bezposrednich kontraktow..." << std::endl;

    MockPlayerForBot mockPlayer;
    MockInventoryForBot mockInventory;
    MockCombatForBot mockCombat;
    MockWorldForBot mockWorld;

    // Inicjalizacja AgentMCPBridge
    AgentMCPBridge& bridge = AgentMCPBridge::Instance();
    bridge.Initialize(&mockPlayer, &mockInventory, &mockCombat, &mockWorld);

    // Upewniamy sie, ze BotApi_FFI nie ma wlasnych kontraktow
    Bot_ResetContracts();

    // Funkcje powinny odpytac AgentMCPBridge jako fallback
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Bot_GetPlayerPos(&x, &y, &z);
    assert(x == 1000.0f && y == 2000.0f && z == 50.0f);

    assert(Bot_GetPlayerStatus(5) == 18500); // HP
    assert(Bot_AttackTarget(2001) == true);
    assert(Bot_UseItem(1, 0) == true);

    ActorSnapshot buf[4];
    int count = Bot_GetActorsInRadius(100.0f, buf, 4);
    assert(count >= 1);

    std::cout << "  -> Fallback do AgentMCPBridge: POPRAWNY" << std::endl;
}

int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << " START TESTU C33: BOT C-ABI BRIDGE (ZERO-SOL2 / ZERO-GC)" << std::endl;
    std::cout << "======================================================" << std::endl;

    TestBinaryLayoutAndPacking();
    TestPlayerPositionAndStatus();
    TestActorsInRadiusZeroAllocation();
    TestGroundItems();
    TestActionsMoveAttackUse();
    TestWindowVisibilityAndThrottle();
    TestFallbackViaAgentMCPBridge();

    std::cout << "======================================================" << std::endl;
    std::cout << " WSZYSTKIE TESTY BOT C-ABI BRIDGE ZAKONCZONE SUKCESEM " << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
