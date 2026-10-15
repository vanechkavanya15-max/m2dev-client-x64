#include <gtest/gtest.h>
#include "../src/Client/Gameplay/MovementRateLimiter.h"
#include <chrono>

using namespace Client::Gameplay;

TEST(MovementRateLimiterTest, InitialStateAllowsMovement) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(std::chrono::steady_clock::now()));
}

TEST(MovementRateLimiterTest, BuffersMovementsWithinInterval) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_FALSE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(50)));
    EXPECT_FALSE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(150)));
}

TEST(MovementRateLimiterTest, AllowsMovementAfterInterval) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(251)));
}

TEST(MovementRateLimiterTest, ForcedSendAlwaysAllows) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(50), true));
}

TEST(MovementRateLimiterTest, UpdatesLastSendTimeOnForcedSend) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(100), true));
    EXPECT_FALSE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(150))); // Only 50ms since forced send
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(351))); // 251ms since forced send
}

TEST(MovementRateLimiterTest, ResetClearsState) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_FALSE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(100)));
    
    limiter.Reset();
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(100))); // Should be true because state was reset
}

TEST(MovementRateLimiterTest, CanChangeInterval) {
    MovementRateLimiter limiter(std::chrono::milliseconds(250));
    auto now = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now));
    EXPECT_FALSE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(150)));
    
    limiter.SetInterval(std::chrono::milliseconds(100));
    EXPECT_EQ(limiter.GetInterval(), std::chrono::milliseconds(100));
    EXPECT_TRUE(limiter.ShouldSendMovementPacket(now + std::chrono::milliseconds(150))); // Now 150 > 100
}
