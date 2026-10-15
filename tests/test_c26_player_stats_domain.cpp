#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../src/Client/Gameplay/PlayerStatsDomain.h"
#include <thread>
#include <vector>

using namespace Client::Gameplay;

// Points based on packet.h values mapped in PlayerStatsDomain.cpp
constexpr uint32_t POINT_LEVEL = 1;
constexpr uint32_t POINT_EXP = 3;
constexpr uint32_t POINT_HP = 5;
constexpr uint32_t POINT_MAX_HP = 6;
constexpr uint32_t POINT_SP = 7;
constexpr uint32_t POINT_MAX_SP = 8;
constexpr uint32_t POINT_GOLD = 11;

TEST_CASE("PlayerStatsDomain - Basic Points") {
    PlayerStatsDomain stats;

    SUBCASE("Default values should be zero") {
        CHECK(stats.GetPoint(POINT_LEVEL) == 0);
        CHECK(stats.GetLevel() == 0);
        CHECK(stats.GetHP() == 0);
        CHECK(stats.GetGold() == 0);
        CHECK(stats.GetExp() == 0);
    }

    SUBCASE("Set and Get normal values") {
        stats.SetPoint(POINT_LEVEL, 42);
        CHECK(stats.GetPoint(POINT_LEVEL) == 42);
        CHECK(stats.GetLevel() == 42);

        stats.SetPoint(POINT_GOLD, 999999999999);
        CHECK(stats.GetPoint(POINT_GOLD) == 999999999999);
        CHECK(stats.GetGold() == 999999999999);
        
        stats.SetPoint(POINT_EXP, 1234567890);
        CHECK(stats.GetExp() == 1234567890);
    }
}

TEST_CASE("PlayerStatsDomain - HP and SP clamping") {
    PlayerStatsDomain stats;

    SUBCASE("Negative HP is clamped to 0") {
        stats.SetPoint(POINT_HP, -50);
        CHECK(stats.GetHP() == 0);
    }

    SUBCASE("Negative SP is clamped to 0") {
        stats.SetPoint(POINT_SP, -10);
        CHECK(stats.GetSP() == 0);
    }

    SUBCASE("Positive HP and SP are set correctly") {
        stats.SetPoint(POINT_HP, 1500);
        CHECK(stats.GetHP() == 1500);

        stats.SetPoint(POINT_SP, 300);
        CHECK(stats.GetSP() == 300);
    }
}

TEST_CASE("PlayerStatsDomain - Status Points") {
    PlayerStatsDomain stats;

    SUBCASE("Set and Get valid status points") {
        stats.SetStatusPoint(10, 50);
        CHECK(stats.GetStatusPoint(10) == 50);
    }

    SUBCASE("Out of bounds status points are ignored/safe") {
        stats.SetStatusPoint(300, 99);
        CHECK(stats.GetStatusPoint(300) == 0);
    }
}

TEST_CASE("PlayerStatsDomain - Thread Safety") {
    PlayerStatsDomain stats;

    auto writer = [&stats]() {
        for (int i = 0; i < 1000; ++i) {
            stats.SetPoint(POINT_HP, i);
        }
    };

    auto reader = [&stats]() {
        for (int i = 0; i < 1000; ++i) {
            (void)stats.GetHP();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(writer);
        threads.emplace_back(reader);
    }

    for (auto& t : threads) {
        t.join();
    }
    
    // HP must be >= 0
    CHECK(stats.GetHP() >= 0);
}
