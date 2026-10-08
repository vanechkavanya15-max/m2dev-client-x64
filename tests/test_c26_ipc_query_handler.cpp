#include <iostream>
#include <cassert>
#include <string>
#include "../src/Client/IPC/IPCQueryHandler.h"
#include "../src/Client/Core/WorldContext.h"

using namespace Client::IPC;
using namespace Client::Core;

void TestPlayerState() {
    WorldContext ctx;
    ctx.currentHp = 100;
    ctx.maxHp = 200;
    ctx.currentSp = 50;
    ctx.maxSp = 100;
    ctx.currentExp = 1500;
    ctx.posX = 10.5f;
    ctx.posY = 20.5f;
    ctx.posZ = 30.5f;
    ctx.isDead = false;

    std::string json = IPCQueryHandler::HandleQueryPlayerState(ctx);
    std::string expected = R"({"hp":100,"max_hp":200,"sp":50,"max_sp":100,"exp":1500,"position":{"x":10.5,"y":20.5,"z":30.5},"is_dead":false})";
    assert(json == expected);

    ctx.isDead = true;
    json = IPCQueryHandler::HandleQueryPlayerState(ctx);
    expected = R"({"hp":100,"max_hp":200,"sp":50,"max_sp":100,"exp":1500,"position":{"x":10.5,"y":20.5,"z":30.5},"is_dead":true})";
    assert(json == expected);

    std::cout << "TestPlayerState passed." << std::endl;
}

void TestInventory() {
    WorldContext ctx;
    ctx.inventory.push_back({0, 11299, 1}); // Armor
    ctx.inventory.push_back({1, 2799, 200}); // Potions

    std::string json = IPCQueryHandler::HandleQueryInventory(ctx);
    std::string expected = R"({"inventory":[{"slot":0,"vnum":11299,"count":1},{"slot":1,"vnum":2799,"count":200}]})";
    assert(json == expected);

    ctx.inventory.clear();
    json = IPCQueryHandler::HandleQueryInventory(ctx);
    expected = R"({"inventory":[]})";
    assert(json == expected);

    std::cout << "TestInventory passed." << std::endl;
}

void TestSurroundings() {
    WorldContext ctx;
    ctx.posX = 0.0f;
    ctx.posY = 0.0f;
    ctx.posZ = 0.0f;

    ctx.entities.push_back({1001, 5.0f, 0.0f, 0.0f, true}); // distance 5
    ctx.entities.push_back({1002, 10.0f, 0.0f, 0.0f, false}); // distance 10
    ctx.entities.push_back({1003, 20.0f, 0.0f, 0.0f, true}); // distance 20

    std::string json = IPCQueryHandler::HandleQuerySurroundings(ctx, 15.0f);
    std::string expected = R"({"surroundings":[{"vid":1001,"x":5,"y":0,"z":0,"is_hostile":true},{"vid":1002,"x":10,"y":0,"z":0,"is_hostile":false}]})";
    
    assert(json == expected);

    json = IPCQueryHandler::HandleQuerySurroundings(ctx, 2.0f);
    expected = R"({"surroundings":[]})";
    assert(json == expected);

    std::cout << "TestSurroundings passed." << std::endl;
}

int main() {
    TestPlayerState();
    TestInventory();
    TestSurroundings();
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
