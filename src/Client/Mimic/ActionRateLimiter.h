#pragma once

#include "../Core/Result.h"
#include "../Core/DomainCommands.h"
#include <chrono>
#include <unordered_map>
#include <cstdint>

namespace Client::Mimic {

enum class ActionType : uint8_t {
    Attack,
    Move,
    Pickup,
    UsePotion,
    CastSkill
};

class ActionRateLimiter {
public:
    ActionRateLimiter() = default;
    ~ActionRateLimiter() = default;

    /// Configures the rate limit for a specific action type.
    /// @param type The action type.
    /// @param maxTokens Maximum number of tokens the bucket can hold.
    /// @param refillRate Refill rate in milliseconds (1 token per refillRate ms).
    void Configure(ActionType type, uint32_t maxTokens, std::chrono::milliseconds refillRate);

    /// Attempts to consume a token for the specified action type.
    /// @param type The action type.
    /// @param now The current time point (for dependency injection/testing).
    /// @return Result containing void on success, or CommandError::RateLimited if no tokens are available.
    [[nodiscard]] Core::Result<void, Core::CommandError> TryAction(
        ActionType type, 
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()
    );

private:
    struct TokenBucket {
        uint32_t tokens{0};
        uint32_t maxTokens{0};
        std::chrono::milliseconds refillRate{0};
        std::chrono::steady_clock::time_point lastRefillTime;
    };

    std::unordered_map<ActionType, TokenBucket> m_buckets;
};

} // namespace Client::Mimic
