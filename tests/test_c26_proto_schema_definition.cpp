#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/Data/ProtoSchemaDefinition.h"

using namespace Client::Data;

TEST_CASE("ProtoSchemaDefinitionTest - ItemProtoInitializationAndComparisons") {
    ItemProtoRecord item1{};
    item1.vnum = EterBase::ItemVnum{10};
    item1.buyPrice = 100;
    item1.sellPrice = 50;

    ItemProtoRecord item2{};
    item2.vnum = EterBase::ItemVnum{10};
    item2.buyPrice = 100;
    item2.sellPrice = 50;

    ItemProtoRecord item3{};
    item3.vnum = EterBase::ItemVnum{11};
    item3.buyPrice = 100;
    item3.sellPrice = 50;

    CHECK(item1 == item2);
    CHECK(item1 != item3);
    CHECK(item1 < item3);
}

TEST_CASE("ProtoSchemaDefinitionTest - ItemProtoInvariants") {
    ItemProtoRecord item{};
    item.vnum = EterBase::ItemVnum{0};
    auto res1 = item.Invariants();
    CHECK(res1.has_value() == false);
    CHECK(res1.error() == "Item VNUM cannot be 0");

    item.vnum = EterBase::ItemVnum{10};
    item.buyPrice = 50;
    item.sellPrice = 100;
    auto res2 = item.Invariants();
    CHECK(res2.has_value() == false);
    CHECK(res2.error() == "Sell price cannot be greater than buy price");

    item.buyPrice = 100;
    item.sellPrice = 50;
    auto res3 = item.Invariants();
    CHECK(res3.has_value() == true);
}

TEST_CASE("ProtoSchemaDefinitionTest - MobProtoInitializationAndComparisons") {
    MobProtoRecord mob1{};
    mob1.vnum = EterBase::EntityId{101};
    mob1.goldMin = 10;
    mob1.goldMax = 20;

    MobProtoRecord mob2{};
    mob2.vnum = EterBase::EntityId{101};
    mob2.goldMin = 10;
    mob2.goldMax = 20;

    MobProtoRecord mob3{};
    mob3.vnum = EterBase::EntityId{102};
    mob3.goldMin = 10;
    mob3.goldMax = 20;

    CHECK(mob1 == mob2);
    CHECK(mob1 != mob3);
    CHECK(mob1 < mob3);
}

TEST_CASE("ProtoSchemaDefinitionTest - MobProtoInvariants") {
    MobProtoRecord mob{};
    mob.vnum = EterBase::EntityId{0};
    auto res1 = mob.Invariants();
    CHECK(res1.has_value() == false);
    CHECK(res1.error() == "Mob VNUM cannot be 0");

    mob.vnum = EterBase::EntityId{101};
    mob.goldMin = 50;
    mob.goldMax = 20;
    auto res2 = mob.Invariants();
    CHECK(res2.has_value() == false);
    CHECK(res2.error() == "Min gold cannot be greater than max gold");

    mob.goldMin = 10;
    mob.goldMax = 20;
    auto res3 = mob.Invariants();
    CHECK(res3.has_value() == true);
}

TEST_CASE("ProtoSchemaDefinitionTest - SchemaHashesAreDeterministic") {
    uint64_t itemHash1 = ProtoSchemaDefinition::GetItemProtoSchemaHash();
    uint64_t itemHash2 = ProtoSchemaDefinition::GetItemProtoSchemaHash();
    CHECK(itemHash1 == itemHash2);
    CHECK(itemHash1 != 0);

    uint64_t mobHash1 = ProtoSchemaDefinition::GetMobProtoSchemaHash();
    uint64_t mobHash2 = ProtoSchemaDefinition::GetMobProtoSchemaHash();
    CHECK(mobHash1 == mobHash2);
    CHECK(mobHash1 != 0);
    CHECK(itemHash1 != mobHash1);
}
