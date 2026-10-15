#include "ActionRateLimiter.h"
#include <algorithm>

namespace Client::Mimic {

void ActionRateLimiter::Configure(ActionType type, uint32_t maxTokens, std::chrono::milliseconds refillRate) {
    auto& bucket = m_buckets[type];
    bucket.maxTokens = maxTokens;
    bucket.tokens = maxTokens;
    bucket.refillRate = refillRate;
    bucket.lastRefillTime = std::chrono::steady_clock::now();
}

Core::Result<void, Core::CommandError> ActionRateLimiter::TryAction(
    ActionType type, 
    std::chrono::steady_clock::time_point now) 
{
    auto it = m_buckets.find(type);
    if (it == m_buckets.end()) {
        // If not configured, allow by default or we could deny. Allowing is safer for unconfigured actions.
        // Let's assume unconfigured actions are not rate-limited.
        return {};
    }

    auto& bucket = it->second;

    if (bucket.refillRate.count() > 0) {
        auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(now - bucket.lastRefillTime);
        if (timePassed >= bucket.refillRate) {
            uint32_t tokensToAdd = static_cast<uint32_t>(timePassed.count() / bucket.refillRate.count());
            bucket.tokens = std::min(bucket.maxTokens, bucket.tokens + tokensToAdd);
            // Advance lastRefillTime by the exact amount of time used to generate the tokens
            bucket.lastRefillTime += bucket.refillRate * tokensToAdd;
        }
    }

    if (bucket.tokens > 0) {
        bucket.tokens--;
        return {};
    }

    return std::unexpected(Core::CommandError::RateLimited);
}

} // namespace Client::Mimic
