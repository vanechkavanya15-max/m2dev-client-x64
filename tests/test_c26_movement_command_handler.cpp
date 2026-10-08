#include "../src/Client/Gameplay/MovementCommandHandler.cpp"
#include <iostream>
#include <cassert>

using namespace Client::Gameplay;
using namespace Client::Core;

void TestValidWalk() {
    WorldContext context;
    context.Reset();
    
    MovementCommandHandler handler(context, []() { return false; });
    
    MoveCommand cmd;
    cmd.moveType = 0; // Walk
    cmd.destination = {100.0f, 200.0f, 300.0f};
    cmd.rotation = 90.0f;
    
    auto result = handler.Handle(cmd);
    
    assert(result.has_value());
    assert(context.localPlayerCoords.x == 100.0f);
    assert(context.localPlayerCoords.y == 200.0f);
    assert(context.localPlayerCoords.z == 300.0f);
    assert(context.localPlayerRotation == 90.0f);
    std::cout << "TestValidWalk passed.\n";
}

void TestValidRun() {
    WorldContext context;
    context.Reset();
    
    MovementCommandHandler handler(context, []() { return false; });
    
    MoveCommand cmd;
    cmd.moveType = 1; // Run
    cmd.destination = {-50.0f, 10.0f, 20.0f};
    cmd.rotation = 180.0f;
    
    auto result = handler.Handle(cmd);
    
    assert(result.has_value());
    assert(context.localPlayerCoords.x == -50.0f);
    assert(context.localPlayerCoords.y == 10.0f);
    assert(context.localPlayerCoords.z == 20.0f);
    assert(context.localPlayerRotation == 180.0f);
    std::cout << "TestValidRun passed.\n";
}

void TestInvalidMoveType() {
    WorldContext context;
    context.Reset();
    
    MovementCommandHandler handler(context, []() { return false; });
    
    MoveCommand cmd;
    cmd.moveType = 2; // Invalid
    cmd.destination = {0.0f, 0.0f, 0.0f};
    
    auto result = handler.Handle(cmd);
    
    assert(!result.has_value());
    assert(result.error() == CommandError::InvalidParameter);
    std::cout << "TestInvalidMoveType passed.\n";
}

void TestDeadPlayer() {
    WorldContext context;
    context.Reset();
    context.isDead = true;
    
    MovementCommandHandler handler(context, []() { return false; });
    
    MoveCommand cmd;
    cmd.moveType = 0;
    cmd.destination = {10.0f, 10.0f, 10.0f};
    
    auto result = handler.Handle(cmd);
    
    assert(!result.has_value());
    assert(result.error() == CommandError::MovementBlocked);
    std::cout << "TestDeadPlayer passed.\n";
}

void TestStunnedPlayer() {
    WorldContext context;
    context.Reset();
    
    MovementCommandHandler handler(context, []() { return true; }); // Stunned
    
    MoveCommand cmd;
    cmd.moveType = 0;
    cmd.destination = {10.0f, 10.0f, 10.0f};
    
    auto result = handler.Handle(cmd);
    
    assert(!result.has_value());
    assert(result.error() == CommandError::MovementBlocked);
    std::cout << "TestStunnedPlayer passed.\n";
}

int main() {
    TestValidWalk();
    TestValidRun();
    TestInvalidMoveType();
    TestDeadPlayer();
    TestStunnedPlayer();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
