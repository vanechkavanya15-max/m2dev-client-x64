#include <gtest/gtest.h>
#include "../src/Client/Network/PendingSpawnRegistry.h"
#include <thread>

using namespace Client::Network;
using namespace EterBase;

class PendingSpawnRegistryTest : public ::testing::Test {
protected:
    PendingSpawnRegistry registry;
    EntityId testVid{12345};
    TPacketGCCharacterAdd spawnPacket{};
    TPacketGCCharacterAdditionalInfo infoPacket{};

    void SetUp() override {
        spawnPacket.dwVID = testVid.get();
        spawnPacket.x = 100;
        spawnPacket.y = 200;
        
        infoPacket.dwVID = testVid.get();
        infoPacket.dwLevel = 50;
    }
};

TEST_F(PendingSpawnRegistryTest, RegisterSpawn_Success) {
    auto result = registry.RegisterSpawn(testVid, spawnPacket);
    EXPECT_TRUE(result.has_value());
}

TEST_F(PendingSpawnRegistryTest, RegisterSpawn_AlreadyExists) {
    registry.RegisterSpawn(testVid, spawnPacket);
    auto result = registry.RegisterSpawn(testVid, spawnPacket);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EntityError::AlreadyExists);
}

TEST_F(PendingSpawnRegistryTest, RegisterAdditionalInfo_Success) {
    registry.RegisterSpawn(testVid, spawnPacket);
    auto result = registry.RegisterAdditionalInfo(testVid, infoPacket);
    
    EXPECT_TRUE(result.has_value());
}

TEST_F(PendingSpawnRegistryTest, RegisterAdditionalInfo_NotFound) {
    auto result = registry.RegisterAdditionalInfo(testVid, infoPacket);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EntityError::NotFound);
}

TEST_F(PendingSpawnRegistryTest, TakeSpawn_Success) {
    registry.RegisterSpawn(testVid, spawnPacket);
    registry.RegisterAdditionalInfo(testVid, infoPacket);
    
    auto result = registry.TakeSpawn(testVid);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().spawnPacket.dwVID, testVid.get());
    ASSERT_TRUE(result.value().additionalInfo.has_value());
    EXPECT_EQ(result.value().additionalInfo->dwLevel, 50);
    
    // Ensure it was removed
    auto result2 = registry.TakeSpawn(testVid);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), EntityError::NotFound);
}

TEST_F(PendingSpawnRegistryTest, TakeSpawn_Incomplete) {
    registry.RegisterSpawn(testVid, spawnPacket);
    
    auto result = registry.TakeSpawn(testVid);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EntityError::InvalidType);
}

TEST_F(PendingSpawnRegistryTest, TakeSpawn_NotFound) {
    auto result = registry.TakeSpawn(testVid);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EntityError::NotFound);
}

TEST_F(PendingSpawnRegistryTest, Cleanup_PurgesOldEntries) {
    registry.RegisterSpawn(testVid, spawnPacket);
    
    // Wait for slightly more than 50ms
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    
    registry.Cleanup(std::chrono::milliseconds(50));
    
    // Entry should be purged
    auto result = registry.RegisterAdditionalInfo(testVid, infoPacket);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), EntityError::NotFound);
}

TEST_F(PendingSpawnRegistryTest, Cleanup_KeepsNewEntries) {
    registry.RegisterSpawn(testVid, spawnPacket);
    
    registry.Cleanup(std::chrono::milliseconds(5000));
    
    // Entry should still exist
    auto result = registry.RegisterAdditionalInfo(testVid, infoPacket);
    EXPECT_TRUE(result.has_value());
}
