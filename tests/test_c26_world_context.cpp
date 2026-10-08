#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <thread>
#include <vector>
#include <chrono>

#include "../src/Client/Core/WorldContext.h"

using namespace Client::Core;
using namespace Client::Gameplay;
using Client::World::ActorRecord;
using Client::World::ActorRegistry;
using Client::World::SpatialHashGrid;

TEST_CASE("WorldContext - Bezposrednia integracja domen i stan poczatkowy") {
    WorldContext ctx;

    SUBCASE("Wszystkie instancje domen C++23 sa prawidlowo zainicjalizowane") {
        // InventoryDomain
        CHECK(ctx.inventory.FindEmptyCell() == 0);

        // PlayerStatsDomain
        CHECK(ctx.stats.GetHP() == 0);
        CHECK(ctx.stats.GetMaxHP() == 0);
        CHECK(ctx.stats.GetSP() == 0);
        CHECK(ctx.stats.GetGold() == 0);

        // SkillDomain
        CHECK_FALSE(ctx.skills.HasSkill(1));

        // QuickslotDomain
        CHECK(ctx.quickslot.GetPage() == 0);
        auto q0 = ctx.quickslot.GetSlot(0);
        REQUIRE(q0.has_value());
        CHECK(q0->IsEmpty());

        // ActorRegistry
        CHECK(ctx.actors.Count() == 0);
        CHECK(ctx.actors.GetMainActorVid().value() == 0);

        // SpatialHashGrid
        CHECK(ctx.spatialGrid.Count() == 0);

        // Primitives
        CHECK(ctx.currentHp == 0);
        CHECK(ctx.maxHp == 0);
        CHECK(ctx.currentSp == 0);
        CHECK(ctx.maxSp == 0);
        CHECK(ctx.currentGold == 0);
        CHECK_FALSE(ctx.isDead);
        CHECK_FALSE(ctx.IsAlive());
    }
}

TEST_CASE("WorldContext - Inicjalizacja lokalnego gracza i synchronizacja przestrzenna") {
    WorldContext ctx;
    ctx.Reset();

    EntityVid playerVid{1001};
    ctx.SetLocalPlayer(playerVid, 1000.0f, 2000.0f, 50.0f, 45.0f, "WarriorHero");

    CHECK(ctx.localPlayerVid == playerVid);
    CHECK(ctx.posX == 1000.0f);
    CHECK(ctx.posY == 2000.0f);
    CHECK(ctx.posZ == 50.0f);
    CHECK(ctx.localPlayerRotation == 45.0f);
    CHECK(ctx.localPlayerCoords.x == 1000.0f);
    CHECK(ctx.localPlayerCoords.y == 2000.0f);

    // Weryfikacja w ActorRegistry
    CHECK(ctx.actors.GetMainActorVid() == EterBase::EntityId{1001});
    auto mainActor = ctx.actors.GetActor(EterBase::EntityId{1001});
    REQUIRE(mainActor.has_value());
    CHECK(mainActor->name == "WarriorHero");
    CHECK(mainActor->x == 1000.0f);
    CHECK(mainActor->y == 2000.0f);

    // Weryfikacja w SpatialHashGrid
    CHECK(ctx.spatialGrid.Count() == 1);
    auto queried = ctx.spatialGrid.QueryRadius(1000.0f, 2000.0f, 10.0f);
    REQUIRE(queried.size() == 1);
    CHECK(queried[0] == EterBase::EntityId{1001});

    // Aktualizacja pozycji gracza
    ctx.UpdatePlayerPosition(1050.0f, 2050.0f, 55.0f, 90.0f);
    CHECK(ctx.posX == 1050.0f);
    CHECK(ctx.posY == 2050.0f);

    auto updatedActor = ctx.actors.GetActor(EterBase::EntityId{1001});
    REQUIRE(updatedActor.has_value());
    CHECK(updatedActor->x == 1050.0f);
    CHECK(updatedActor->y == 2050.0f);

    // Stara pozycja w siatce pusta, nowa ma aktora
    CHECK(ctx.spatialGrid.QueryRadius(1000.0f, 2000.0f, 10.0f).empty());
    CHECK(ctx.spatialGrid.QueryRadius(1050.0f, 2050.0f, 10.0f).size() == 1);
}

TEST_CASE("WorldContext - Synchronizacja statystyk i cyklu zycia (Stats + Lifecycle)") {
    WorldContext ctx;
    ctx.Reset();

    EntityVid playerVid{1001};
    ctx.SetLocalPlayer(playerVid, 100.0f, 100.0f, 0.0f, 0.0f, "Hero");

    // Ustawienie HP i MaxHP
    ctx.SetPlayerHp(12500, 15000);
    CHECK(ctx.currentHp == 12500);
    CHECK(ctx.maxHp == 15000);
    CHECK(ctx.stats.GetHP() == 12500);
    CHECK(ctx.stats.GetMaxHP() == 15000);
    CHECK(ctx.GetPoint(5) == 12500);
    CHECK(ctx.GetPoint(6) == 15000);
    CHECK(ctx.IsAlive());
    CHECK_FALSE(ctx.isDead);

    // Ustawienie SP i MaxSP
    ctx.SetPlayerSp(3200, 4000);
    CHECK(ctx.currentSp == 3200);
    CHECK(ctx.maxSp == 4000);
    CHECK(ctx.stats.GetSP() == 3200);
    CHECK(ctx.stats.GetMaxSP() == 4000);
    CHECK(ctx.GetPoint(7) == 3200);
    CHECK(ctx.GetPoint(8) == 4000);

    // Synchronizacja zlota przez SetPoint
    ctx.SetPoint(11, 750000);
    CHECK(ctx.currentGold == 750000);
    CHECK(ctx.stats.GetGold() == 750000);
    CHECK(ctx.GetPoint(11) == 750000);

    // Smierc postaci
    ctx.SetPlayerDead(true);
    CHECK(ctx.isDead);
    CHECK(ctx.currentHp == 0);
    CHECK_FALSE(ctx.IsAlive());
    CHECK(ctx.actors.IsDead(EterBase::EntityId{1001}));

    // Ozywienie
    ctx.SetPlayerHp(5000);
    CHECK_FALSE(ctx.isDead);
    CHECK(ctx.currentHp == 5000);
    CHECK(ctx.IsAlive());
    CHECK_FALSE(ctx.actors.IsDead(EterBase::EntityId{1001}));
}

TEST_CASE("WorldContext - Koordynacja aktorow i zapytan przestrzennych (Actors + SpatialGrid)") {
    WorldContext ctx;
    ctx.Reset();

    EntityVid playerVid{1001};
    ctx.SetLocalPlayer(playerVid, 1000.0f, 1000.0f, 0.0f, 0.0f, "Hero");

    // Rejestracja wrogich potworow i NPC
    ActorRecord mob1{
        .vid = EterBase::EntityId{2001},
        .race = 101, // Dziki Pies
        .type = 1,   // Monster
        .x = 1050.0f,
        .y = 1000.0f,
        .z = 0.0f,
        .rotation = 0.0f,
        .name = "WildDog_1",
        .guildId = 0,
        .empire = 0,
        .isDead = false
    };

    ActorRecord mob2{
        .vid = EterBase::EntityId{2002},
        .race = 101,
        .type = 1,
        .x = 1300.0f,
        .y = 1000.0f,
        .z = 0.0f,
        .rotation = 0.0f,
        .name = "WildDog_2",
        .guildId = 0,
        .empire = 0,
        .isDead = false
    };

    ActorRecord npc{
        .vid = EterBase::EntityId{3001},
        .race = 20001, // Handlarz
        .type = 2,     // NPC
        .x = 3500.0f,
        .y = 3500.0f,
        .z = 0.0f,
        .rotation = 0.0f,
        .name = "Shopkeeper",
        .guildId = 0,
        .empire = 1,
        .isDead = false
    };

    CHECK(ctx.RegisterActor(mob1));
    CHECK(ctx.RegisterActor(mob2));
    CHECK(ctx.RegisterActor(npc));
    CHECK(ctx.actors.Count() == 4); // Gracz + 2 moby + NPC
    CHECK(ctx.spatialGrid.Count() == 4);

    // Zapytanie o aktorow w promieniu 100 jednostek wokol gracza (1000, 1000)
    // Powinno zwrocic gracza (1000, 1000) i mob1 (1050, 1000, odleglosc 50)
    auto nearby100 = ctx.FindNearbyActors(100.0f);
    CHECK(nearby100.size() == 2);

    // Zapytanie o najblizszego aktora z wylaczeniem gracza (excludeSelf = true)
    auto nearest = ctx.FindNearestActor(500.0f, true);
    REQUIRE(nearest.has_value());
    CHECK(nearest->vid == EterBase::EntityId{2001});
    CHECK(nearest->name == "WildDog_1");

    // Przemieszczenie mob1 dalej niz mob2
    ctx.UpdateActorPosition(EntityVid{2001}, 1500.0f, 1000.0f, 0.0f, 0.0f);
    auto nearestAfterMove = ctx.FindNearestActor(500.0f, true);
    REQUIRE(nearestAfterMove.has_value());
    CHECK(nearestAfterMove->vid == EterBase::EntityId{2002}); // Teraz mob2 (odl. 300) jest blizej

    // Wyrejestrowanie mob2
    CHECK(ctx.UnregisterActor(EntityVid{2002}));
    CHECK(ctx.actors.Count() == 3);
    CHECK(ctx.spatialGrid.Count() == 3);
    CHECK_FALSE(ctx.actors.FindActor(EterBase::EntityId{2002}));
}

TEST_CASE("WorldContext - Koordynacja umiejetnosci i zuzycia many (Skills + SP)") {
    WorldContext ctx;
    ctx.Reset();

    ctx.SetPlayerHp(10000, 10000);
    ctx.SetPlayerSp(200, 1000);

    // Definiujemy skill: Aura Miecza (ID 1)
    SkillData auraDef{
        .id = 1,
        .type = SkillType::Active,
        .target = SkillTarget::Self,
        .baseSPCost = 50,
        .spMultiplier = 2.0f,
        .name = "Aura of Sword"
    };
    ctx.skills.DefineSkill(1, auraDef);

    // Gracz jeszcze nie zna umiejetnosci
    CHECK_FALSE(ctx.CanUseSkill(1));
    CHECK_FALSE(ctx.UseSkillMs(1, 2000));

    // Gracz uczy sie na poziomie 10: koszt = 50 + 10 * 2.0 * 1.0 = 70 SP
    ctx.skills.RegisterSkill(1, 10);
    CHECK(ctx.skills.HasSkill(1));
    CHECK(ctx.skills.CalculateSPCost(1) == 70);

    // Uzycie skilla z 200 SP
    CHECK(ctx.CanUseSkill(1));
    CHECK(ctx.UseSkillMs(1, 2000)); // 2 sekundy cooldownu

    // SP zostalo pobrane (200 - 70 = 130) zarowno w kontekscie, jak i domenie stats
    CHECK(ctx.currentSp == 130);
    CHECK(ctx.stats.GetSP() == 130);

    // Umiejetnosc jest teraz na cooldownie
    CHECK_FALSE(ctx.CanUseSkill(1));
    CHECK_FALSE(ctx.UseSkillMs(1, 2000));

    // Reset cooldownu
    ctx.skills.ResetCooldown(1);
    CHECK(ctx.CanUseSkill(1));

    // Uzywamy ponownie (130 - 70 = 60 SP)
    CHECK(ctx.UseSkillMs(1, 2000));
    CHECK(ctx.currentSp == 60);

    // Kolejny raz - brak many (koszt 70, gracz ma 60)
    ctx.skills.ResetCooldown(1);
    CHECK_FALSE(ctx.CanUseSkill(1));
    CHECK_FALSE(ctx.UseSkillMs(1, 2000));
    CHECK(ctx.currentSp == 60); // Niezmienione
}

TEST_CASE("WorldContext - Ekwipunek i powiazanie z paskiem szybkiego dostepu (Inventory + Quickslot)") {
    WorldContext ctx;
    ctx.Reset();

    // Dodanie przedmiotu do inwentarza (Miecz+9 w slocie 5)
    ItemData sword{
        .vnum = EterBase::ItemVnum{19},
        .count = 1,
        .size = ItemSize{1, 2}
    };
    auto setItemRes = ctx.inventory.SetItem(InventoryWindow::Inventory, EterBase::ItemSlot{5}, sword);
    REQUIRE(setItemRes.has_value());

    // Definiujemy i rejestrujemy skill (Wir Miecza, ID 3)
    SkillData whirlDef{.id = 3, .type = SkillType::Active, .baseSPCost = 40, .name = "Whirlwind"};
    ctx.skills.DefineSkill(3, whirlDef);
    ctx.skills.RegisterSkill(3, 1);

    // Powiazanie z paskiem szybkiego dostepu
    CHECK(ctx.BindQuickslotItem(0, 5));  // Quickslot 0 -> Miecz w inv slot 5
    CHECK(ctx.BindQuickslotSkill(1, 3)); // Quickslot 1 -> Skill Wir Miecza (ID 3)

    // Bindowanie nieistniejacego przedmiotu / skilla powinno zawiesc
    CHECK_FALSE(ctx.BindQuickslotItem(2, 99)); // Slot 99 jest pusty
    CHECK_FALSE(ctx.BindQuickslotSkill(3, 999)); // Skill 999 nieznany

    // Weryfikacja zawartosci paska szybkiego dostepu
    auto qs0 = ctx.quickslot.GetSlot(0);
    REQUIRE(qs0.has_value());
    CHECK(qs0->type == 1); // Item
    CHECK(qs0->position == 5);

    auto qs1 = ctx.quickslot.GetSlot(1);
    REQUIRE(qs1.has_value());
    CHECK(qs1->type == 2); // Skill
    CHECK(qs1->position == 3);
}

TEST_CASE("WorldContext - Wielowatkowosc i rownolegly dostep (Multithreading Concurrency)") {
    WorldContext ctx;
    ctx.Reset();

    ctx.SetLocalPlayer(EntityVid{1001}, 500.0f, 500.0f, 0.0f, 0.0f, "ConcurrentUser");
    ctx.SetPlayerHp(10000, 10000);
    ctx.SetPlayerSp(10000, 10000);

    constexpr int ITERATIONS = 150;
    std::atomic<bool> startFlag{false};

    // Watek 1: Przemieszczanie gracza i aktualizacja w siatce
    std::thread t1([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < ITERATIONS; ++i) {
            float pos = 500.0f + static_cast<float>(i);
            ctx.UpdatePlayerPosition(pos, pos, 0.0f, 0.0f);
        }
    });

    // Watek 2: Zmiany statystyk (HP/SP/Gold)
    std::thread t2([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < ITERATIONS; ++i) {
            ctx.SetPoint(11, 1000 + i);
            ctx.SetPlayerHp(9000 + (i % 1000));
        }
    });

    // Watek 3: Dodawanie i usuwanie obcych aktorow
    std::thread t3([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < ITERATIONS; ++i) {
            ActorRecord rec{
                .vid = EterBase::EntityId{static_cast<uint32_t>(5000 + i)},
                .race = 101,
                .type = 1,
                .x = 510.0f,
                .y = 510.0f,
                .z = 0.0f,
                .rotation = 0.0f,
                .name = "SpawnedMob",
                .guildId = 0,
                .empire = 0,
                .isDead = false
            };
            ctx.RegisterActor(rec);
            ctx.UnregisterActor(EntityVid{static_cast<uint32_t>(5000 + i)});
        }
    });

    // Watek 4: Zapytania przestrzenne i odczyty
    std::thread t4([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < ITERATIONS; ++i) {
            auto nearby = ctx.FindNearbyActors(100.0f);
            auto gold = ctx.GetPoint(11);
            (void)nearby;
            (void)gold;
        }
    });

    startFlag.store(true);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // Po zakonczeniu watkow stan jest stabilny
    CHECK(ctx.IsAlive());
    CHECK(ctx.actors.Count() >= 1); // Lokalny gracz pozostaje zarejestrowany
}

TEST_CASE("WorldContext - Pelny Reset() czyszczacy wszystkie domeny i stan pamieci") {
    WorldContext ctx;
    ctx.Reset();

    // 1. Zasilamy wszystkie podsystemy danymi
    ctx.SetLocalPlayer(EntityVid{1001}, 123.0f, 456.0f, 78.0f, 90.0f, "Hero");
    ctx.SetPlayerHp(5000, 8000);
    ctx.SetPlayerSp(1000, 2000);
    ctx.SetPoint(11, 999999);

    ItemData potion{.vnum = EterBase::ItemVnum{27001}, .count = 20};
    CHECK(ctx.inventory.SetItem(InventoryWindow::Inventory, EterBase::ItemSlot{0}, potion).has_value());

    SkillData skill{.id = 5, .type = SkillType::Active, .baseSPCost = 10};
    ctx.skills.DefineSkill(5, skill);
    ctx.skills.RegisterSkill(5, 5);

    CHECK(ctx.BindQuickslotItem(0, 0));

    ActorRecord mob{.vid = EterBase::EntityId{2001}, .race = 101, .type = 1, .x = 130.0f, .y = 460.0f, .name = "Mob"};
    CHECK(ctx.RegisterActor(mob));

    CHECK(ctx.actors.Count() == 2);
    CHECK(ctx.spatialGrid.Count() == 2);

    // 2. Wywolujemy Reset()
    ctx.Reset();

    // 3. Weryfikujemy pelne wyczyszczenie
    CHECK(ctx.currentHp == 0);
    CHECK(ctx.maxHp == 0);
    CHECK(ctx.currentSp == 0);
    CHECK(ctx.maxSp == 0);
    CHECK(ctx.currentGold == 0);
    CHECK(ctx.GetPoint(11) == 0);
    CHECK(ctx.posX == 0.0f);
    CHECK(ctx.posY == 0.0f);
    CHECK(ctx.posZ == 0.0f);
    CHECK(ctx.localPlayerVid.get() == 0);
    CHECK_FALSE(ctx.isDead);
    CHECK_FALSE(ctx.IsAlive());

    // Domeny zresetowane
    CHECK(ctx.actors.Count() == 0);
    CHECK(ctx.spatialGrid.Count() == 0);
    CHECK(ctx.inventory.FindEmptyCell() == 0);
    auto emptyItem = ctx.inventory.GetItem(InventoryWindow::Inventory, EterBase::ItemSlot{0});
    CHECK_FALSE(emptyItem.has_value());
    CHECK(emptyItem.error() == EterBase::InventoryError::SlotEmpty);

    CHECK_FALSE(ctx.skills.HasSkill(5));
    CHECK(ctx.quickslot.GetSlot(0)->IsEmpty());
}
