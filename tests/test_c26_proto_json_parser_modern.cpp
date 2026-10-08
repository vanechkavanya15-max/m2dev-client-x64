#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>
#include <vector>
#include <iostream>
#include <expected>
#include <cstring>

// Stub Windows types and macros
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;

#define ITEM_NAME_MAX_LEN 24
#define ITEM_LIMIT_MAX_NUM 2
#define ITEM_APPLY_MAX_NUM 3
#define ITEM_VALUES_MAX_NUM 6
#define ITEM_SOCKET_MAX_NUM 3

#define CHARACTER_NAME_MAX_LEN 24

// Mock structures to avoid importing entire client headers in this unit test
namespace CItemData {
    typedef struct SItemLimit {
        BYTE bType;
        long lValue;
    } TItemLimit;

    typedef struct SItemApply {
        BYTE bType;
        long lValue;
    } TItemApply;

    typedef struct SItemTable {
        DWORD dwVnum;
        DWORD dwVnumRange;
        char szName[ITEM_NAME_MAX_LEN + 1];
        char szLocaleName[ITEM_NAME_MAX_LEN + 1];
        BYTE bType;
        BYTE bSubType;
        BYTE bWeight;
        BYTE bSize;
        DWORD dwAntiFlags;
        DWORD dwFlags;
        DWORD dwWearFlags;
        DWORD dwImmuneFlag;
        DWORD dwIBuyItemPrice;
        DWORD dwISellItemPrice;
        TItemLimit aLimits[ITEM_LIMIT_MAX_NUM];
        TItemApply aApplies[ITEM_APPLY_MAX_NUM];
        long alValues[ITEM_VALUES_MAX_NUM];
        long alSockets[ITEM_SOCKET_MAX_NUM];
        DWORD dwRefinedVnum;
        WORD wRefineSet;
        BYTE bAlterToMagicItemPct;
        BYTE bSpecular;
        BYTE bGainSocketPct;
    } TItemTable;
}

namespace CPythonNonPlayer {
    enum EMobEnchants { MOB_ENCHANTS_MAX_NUM = 6 }; // Usually 7, we mocked the constant in header parser as 6 or loop based on actual max
    enum EMobResists { MOB_RESISTS_MAX_NUM = 11 }; // Actually 12
    #define MOB_SKILL_MAX_NUM 5

    typedef struct SMobSkillLevel {
        DWORD dwVnum;
        BYTE bLevel;
    } TMobSkillLevel;

    typedef struct SMobTable {
        DWORD dwVnum;
        char szName[CHARACTER_NAME_MAX_LEN + 1]; 
        char szLocaleName[CHARACTER_NAME_MAX_LEN + 1];
        BYTE bType;
        BYTE bRank;
        BYTE bBattleType;
        BYTE bLevel;
        BYTE bSize;
        DWORD dwGoldMin;
        DWORD dwGoldMax;
        DWORD dwExp;
        DWORD dwMaxHP;
        BYTE bRegenCycle;
        BYTE bRegenPercent;
        WORD wDef;
        DWORD dwAIFlag;
        DWORD dwRaceFlag;
        DWORD dwImmuneFlag;
        BYTE bStr, bDex, bCon, bInt;
        DWORD dwDamageRange[2];
        short sAttackSpeed;
        short sMovingSpeed;
        BYTE bAggresiveHPPct;
        WORD wAggressiveSight;
        WORD wAttackRange;
        char cEnchants[MOB_ENCHANTS_MAX_NUM];
        char cResists[MOB_RESISTS_MAX_NUM];
        DWORD dwResurrectionVnum;
        DWORD dwDropItemVnum;
        BYTE bMountCapacity;
        BYTE bOnClickType;
        BYTE bEmpire;
        char szFolder[64 + 1];
        float fDamMultiply;
        DWORD dwSummonVnum;
        DWORD dwDrainSP;
        DWORD dwMonsterColor;
        DWORD dwPolymorphItemVnum;
        TMobSkillLevel Skills[MOB_SKILL_MAX_NUM];
        BYTE bBerserkPoint;
        BYTE bStoneSkinPoint;
        BYTE bGodSpeedPoint;
        BYTE bDeathBlowPoint;
        BYTE bRevivePoint;
    } TMobTable;
}

// Intercept headers
#define GAME_LIB_ITEMDATA_H
#define USERINTERFACE_PYTHONNONPLAYER_H



using namespace Client::Data;

TEST_CASE("ProtoJSON - Parse Valid ItemTable") {
    std::string validJSON = R"([
        {
            "dwVnum": 10,
            "dwVnumRange": 0,
            "szName": "Sword",
            "szLocaleName": "Miecz",
            "bType": 1,
            "bSubType": 2,
            "bWeight": 5,
            "bSize": 2,
            "dwAntiFlags": 0,
            "dwFlags": 0,
            "dwWearFlags": 16,
            "dwImmuneFlag": 0,
            "dwIBuyItemPrice": 100,
            "dwISellItemPrice": 50,
            "aLimits": [{"bType": 1, "lValue": 10}],
            "aApplies": [{"bType": 1, "lValue": 100}],
            "alValues": [10, 20, 30, 40, 50, 60],
            "alSockets": [0, 0, 0],
            "dwRefinedVnum": 11,
            "wRefineSet": 1,
            "bAlterToMagicItemPct": 5,
            "bSpecular": 10,
            "bGainSocketPct": 100
        }
    ])";

    auto res = ProtoJSONParserModern::ParseItemTable(validJSON);
    REQUIRE(res.has_value());
    REQUIRE(res->size() == 1);
    
    const auto& item = (*res)[0];
    CHECK(item.dwVnum == 10);
    CHECK(std::string(item.szName) == "Sword");
    CHECK(item.bType == 1);
    CHECK(item.alValues[0] == 10);
    CHECK(item.dwRefinedVnum == 11);
}

TEST_CASE("ProtoJSON - Parse Valid MobTable") {
    std::string validJSON = R"([
        {
            "dwVnum": 101,
            "szName": "Wild Dog",
            "szLocaleName": "Dziki Pies",
            "bType": 0,
            "bRank": 0,
            "bBattleType": 0,
            "bLevel": 1,
            "bSize": 1,
            "dwGoldMin": 10,
            "dwGoldMax": 20,
            "dwExp": 15,
            "dwMaxHP": 100,
            "bRegenCycle": 3,
            "bRegenPercent": 10,
            "wDef": 5,
            "dwAIFlag": 0,
            "dwRaceFlag": 0,
            "dwImmuneFlag": 0,
            "bStr": 5, "bDex": 5, "bCon": 5, "bInt": 5,
            "dwDamageRange": [10, 15],
            "sAttackSpeed": 100,
            "sMovingSpeed": 100,
            "bAggresiveHPPct": 0,
            "wAggressiveSight": 2000,
            "wAttackRange": 150,
            "cEnchants": [0,0,0,0,0,0],
            "cResists": [0,0,0,0,0,0,0,0,0,0,0],
            "dwResurrectionVnum": 0,
            "dwDropItemVnum": 0,
            "bMountCapacity": 0,
            "bOnClickType": 0,
            "bEmpire": 0,
            "szFolder": "wild_dog",
            "fDamMultiply": 1.0,
            "dwSummonVnum": 0,
            "dwDrainSP": 0,
            "dwMonsterColor": 0,
            "dwPolymorphItemVnum": 0,
            "Skills": [],
            "bBerserkPoint": 0,
            "bStoneSkinPoint": 0,
            "bGodSpeedPoint": 0,
            "bDeathBlowPoint": 0,
            "bRevivePoint": 0
        }
    ])";

    auto res = ProtoJSONParserModern::ParseMobTable(validJSON);
    REQUIRE(res.has_value());
    REQUIRE(res->size() == 1);

    const auto& mob = (*res)[0];
    CHECK(mob.dwVnum == 101);
    CHECK(std::string(mob.szName) == "Wild Dog");
    CHECK(mob.bLevel == 1);
    CHECK(mob.dwDamageRange[0] == 10);
    CHECK(mob.dwDamageRange[1] == 15);
}

TEST_CASE("ProtoJSON - Type Mismatch") {
    // bType should be an integer, we provide string
    std::string invalidJSON = R"([
        {
            "dwVnum": 10,
            "dwVnumRange": 0,
            "szName": "Sword",
            "szLocaleName": "Miecz",
            "bType": "not_an_int"
        }
    ])";

    auto res = ProtoJSONParserModern::ParseItemTable(invalidJSON);
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == ProtoParseError::TypeMismatch);
}

TEST_CASE("ProtoJSON - Missing Field") {
    // Missing dwVnumRange
    std::string invalidJSON = R"([
        {
            "dwVnum": 10,
            "szName": "Sword"
        }
    ])";

    auto res = ProtoJSONParserModern::ParseItemTable(invalidJSON);
    REQUIRE_FALSE(res.has_value());
    CHECK(res.error() == ProtoParseError::MissingField);
}

TEST_CASE("ProtoJSON - Round Trip Export ItemTable") {
    CItemData::TItemTable item = {};
    item.dwVnum = 12345;
    std::strncpy(item.szName, "TestItem", ITEM_NAME_MAX_LEN);
    item.bType = 2;
    item.alValues[0] = 999;
    
    std::vector<CItemData::TItemTable> vec = { item };

    auto exportedResult = ProtoJSONParserModern::ExportItemTableToJSON(vec);
    REQUIRE(exportedResult.has_value());
    
    auto parsedResult = ProtoJSONParserModern::ParseItemTable(exportedResult.value());
    REQUIRE(parsedResult.has_value());
    REQUIRE(parsedResult->size() == 1);

    const auto& parsedItem = (*parsedResult)[0];
    CHECK(parsedItem.dwVnum == 12345);
    CHECK(std::string(parsedItem.szName) == "TestItem");
    CHECK(parsedItem.bType == 2);
    CHECK(parsedItem.alValues[0] == 999);
}
