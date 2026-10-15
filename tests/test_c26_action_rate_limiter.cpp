#include <gtest/gtest.h>
#include "../src/Client/Mimic/ActionRateLimiter.h"
#include "../src/Client/Core/Result.h"
#include "../src/Client/Core/DomainCommands.h"
#include <chrono>

using namespace Client::Mimic;
using namespace Client::Core;

class ActionRateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup initial configuration
        limiter.Configure(ActionType::Attack, 3, std::chrono::milliseconds(100));
        limiter.Configure(ActionType::Move, 5, std::chrono::milliseconds(50));
        limiter.Configure(ActionType::UsePotion, 1, std::chrono::milliseconds(500));
    }

    ActionRateLimiter limiter;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
};

TEST_F(ActionRateLimiterTest, AllowInitialActionsUpToMaxTokens) {
    // Attack has 3 max tokens
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());

    // 4th attack should be rate limited
    auto result = limiter.TryAction(ActionType::Attack, startTime);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::RateLimited);
}

TEST_F(ActionRateLimiterTest, RefillTokensOverTime) {
    // Consume all Attack tokens
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());

    // Move forward 150ms -> should refill 1 token (refill rate is 100ms)
    auto futureTime = startTime + std::chrono::milliseconds(150);
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, futureTime).has_value());

    // Second action should fail again
    auto result = limiter.TryAction(ActionType::Attack, futureTime);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::RateLimited);
}

TEST_F(ActionRateLimiterTest, MaxTokensAreRespected) {
    // Move forward 1000ms -> should not exceed maxTokens
    auto futureTime = startTime + std::chrono::milliseconds(1000);
    
    // Attack has 3 max tokens
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, futureTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, futureTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, futureTime).has_value());

    // 4th should fail
    auto result = limiter.TryAction(ActionType::Attack, futureTime);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CommandError::RateLimited);
}

TEST_F(ActionRateLimiterTest, DifferentActionTypesAreIndependent) {
    // Consume all UsePotion tokens (1 token)
    EXPECT_TRUE(limiter.TryAction(ActionType::UsePotion, startTime).has_value());
    EXPECT_FALSE(limiter.TryAction(ActionType::UsePotion, startTime).has_value());

    // Attack should still be unaffected
    EXPECT_TRUE(limiter.TryAction(ActionType::Attack, startTime).has_value());
}

TEST_F(ActionRateLimiterTest, UnconfiguredActionTypeAllowsAlways) {
    // CastSkill is not configured
    EXPECT_TRUE(limiter.TryAction(ActionType::CastSkill, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::CastSkill, startTime).has_value());
    EXPECT_TRUE(limiter.TryAction(ActionType::CastSkill, startTime).has_value());
}
