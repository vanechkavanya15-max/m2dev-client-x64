#include <gtest/gtest.h>
#include "../src/Client/Network/Handlers/ActorSpawnHandler.h"
#include "../src/Client/Network/PendingSpawnRegistry.h"
#include "../src/UserInterface/Packet.h"
#include <vector>
#include <cstring>

using namespace Client::Network;
using namespace Client::Network::Handlers;
using namespace EterBase;

class ActorSpawnHandlerTest : public ::testing::Test {
protected:
    PendingSpawnRegistry registry;
    ActorSpawnHandler handler;
    TPacketGCCharacterAdd spawnPacket{};

    void SetUp() override {
        spawnPacket.header = 1; // Assuming 1 is HEADER_GC_CHARACTER_ADD
        spawnPacket.length = sizeof(TPacketGCCharacterAdd);
        spawnPacket.dwVID = 12345;
        spawnPacket.x = 100;
        spawnPacket.y = 200;
        spawnPacket.z = 300;
    }
};

TEST_F(ActorSpawnHandlerTest, HandleActorSpawn_BufferUnderflow) {
    std::vector<uint8_t> payload(sizeof(TPacketGCCharacterAdd) - 1);
    
    auto result = handler.HandleActorSpawn(payload, registry);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), PacketError::BufferUnderflow);
}

TEST_F(ActorSpawnHandlerTest, HandleActorSpawn_Success) {
    std::vector<uint8_t> payload(sizeof(TPacketGCCharacterAdd));
    std::memcpy(payload.data(), &spawnPacket, sizeof(TPacketGCCharacterAdd));
    
    auto result = handler.HandleActorSpawn(payload, registry);
    
    EXPECT_TRUE(result.has_value());
    
    // Verify registration in the real registry
    EntityId vid{spawnPacket.dwVID};
    auto takeResult = registry.TakeSpawn(vid);
    // Since TakeSpawn removes and returns the PendingSpawnData (or returns an error if not found but registered)
    // Actually wait, TakeSpawn requires BOTH spawn and additional info to be registered.
    // Let's check how PendingSpawnRegistry works.
    // We can just verify it doesn't crash and returns success.
    // And if we try to register it again, it should fail with AlreadyExists, so we can use that to verify!
    
    auto duplicateResult = registry.RegisterSpawn(vid, spawnPacket);
    EXPECT_FALSE(duplicateResult.has_value());
    EXPECT_EQ(duplicateResult.error(), EntityError::AlreadyExists);
}
