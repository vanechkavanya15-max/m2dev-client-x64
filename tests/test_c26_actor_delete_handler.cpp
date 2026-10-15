#include "../src/Client/Network/Handlers/ActorDeleteHandler.h"
#include "../src/Client/World/SpatialHashGrid.h"
#include "../src/Client/Network/PendingSpawnRegistry.h"
#include "../src/Client/Network/ActorPacketCodec.h"

#include <iostream>
#include <cassert>

using namespace Client::Network;
using namespace Client::Network::Handlers;
using namespace Client::World;

void Test_ActorDeleteHandler_Success() {
    SpatialHashGrid grid;
    PendingSpawnRegistry registry;

    EntityVid vid{ 1234 };

    // Set up test data
    grid.Insert(vid, 100.0f, 200.0f);
    
    Client::Network::TPacketGCCharacterAdd addPacket{};
    addPacket.dwVID = vid.get();
    registry.RegisterSpawn(vid, addPacket);

    Client::Network::TPacketGCCharacterAdditionalInfo addInfo{};
    addInfo.dwVID = vid.get();
    registry.RegisterAdditionalInfo(vid, addInfo);

    // Ensure it's in the grid
    auto inGrid = grid.QueryRadius(100.0f, 200.0f, 10.0f);
    assert(inGrid.size() == 1);
    assert(inGrid[0] == vid);

    Client::Network::TPacketGCCharacterDelete deletePacket{};
    deletePacket.header = 0; // Assume 0 is valid or not checked in DecodeCharacterDelete for dummy
    deletePacket.dwVID = vid.get();

    auto result = ActorDeleteHandler::Handle(deletePacket, grid, registry);
    assert(result.has_value());

    // Ensure it's removed from grid
    auto inGridAfter = grid.QueryRadius(100.0f, 200.0f, 10.0f);
    assert(inGridAfter.empty());

    // Ensure it's removed from registry (TakeSpawn returns NotFound)
    auto takeResult = registry.TakeSpawn(vid);
    assert(!takeResult.has_value());
    assert(takeResult.error() == EterBase::EntityError::NotFound);
    
    std::cout << "Test_ActorDeleteHandler_Success passed!\n";
}

void Test_ActorDeleteHandler_IncompleteSpawn() {
    SpatialHashGrid grid;
    PendingSpawnRegistry registry;

    EntityVid vid{ 5678 };

    // Register spawn but NO additional info
    Client::Network::TPacketGCCharacterAdd addPacket{};
    addPacket.dwVID = vid.get();
    registry.RegisterSpawn(vid, addPacket);

    Client::Network::TPacketGCCharacterDelete deletePacket{};
    deletePacket.header = 0;
    deletePacket.dwVID = vid.get();

    auto result = ActorDeleteHandler::Handle(deletePacket, grid, registry);
    assert(result.has_value());

    // Ensure it's removed from registry (TakeSpawn returns NotFound, NOT InvalidType)
    auto takeResult = registry.TakeSpawn(vid);
    assert(!takeResult.has_value());
    assert(takeResult.error() == EterBase::EntityError::NotFound);
    
    std::cout << "Test_ActorDeleteHandler_IncompleteSpawn passed!\n";
}

int main() {
    Test_ActorDeleteHandler_Success();
    Test_ActorDeleteHandler_IncompleteSpawn();
    std::cout << "All ActorDeleteHandler tests passed!\n";
    return 0;
}
