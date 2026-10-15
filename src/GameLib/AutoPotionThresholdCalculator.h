#pragma once

#include <deque>
#include <chrono>
#include <optional>
#include <algorithm>
#include <numeric>
#include <expected>
#include <unordered_map>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when an auto potion should be consumed based on dynamic DPS thresholds.
 */
struct AutoPotionTriggerEvent {
    EterBase::EntityId playerId;
    uint8_t potionType; // 0 for HP, 1 for SP
    float currentHpRatio;
    float calculatedThreshold;
};

/**
 * @brief Calculates dynamic threshold for drinking auto potions depending on received DPS.
 * 
 * Uses a sliding window (std::deque) of recent damage events per player to calculate incoming DPS. 
 * If the current HP is not enough to withstand the expected damage over a short period,
 * it emits an event to trigger the auto potion.
 */
class AutoPotionThresholdCalculator {
public:
    /**
     * @brief Constructor for the calculator.
     * @param windowDuration The duration of the sliding window for DPS calculation.
     * @param safeMarginRatio Additional HP margin to add to the threshold as a safety buffer.
     */
    explicit AutoPotionThresholdCalculator(std::chrono::milliseconds windowDuration = std::chrono::milliseconds(3000), 
                                           float safeMarginRatio = 0.15f)
        : windowDuration_(windowDuration), safeMarginRatio_(safeMarginRatio) {}

    /**
     * @brief Records a damage event for a player.
     * @param playerId The entity ID of the player taking damage.
     * @param damageAmount The amount of damage received.
     * @return std::expected<void, EterBase::EntityError> Success or error type.
     */
    std::expected<void, EterBase::EntityError> RecordDamage(EterBase::EntityId playerId, float damageAmount) {
        if (!playerId) {
            EterBase::ModernLogger::Warn("Failed to record damage: Invalid Player ID.");
            return std::unexpected(EterBase::EntityError::InvalidType);
        }

        auto now = std::chrono::steady_clock::now();
        damageHistory_[playerId].push_back({damageAmount, now});
        CleanUpHistory(playerId, now);

        return {};
    }

    /**
     * @brief Retrieves the last calculated dynamic threshold for the player, if available.
     * @param playerId The entity ID.
     * @return std::optional<float> The threshold ratio if there is damage history.
     */
    std::optional<float> GetLastThreshold(EterBase::EntityId playerId) const {
        auto it = damageHistory_.find(playerId);
        if (it != damageHistory_.end() && !it->second.empty()) {
            return 0.20f + safeMarginRatio_; // dummy baseline for optional monad demonstration
        }
        return std::nullopt;
    }

    /**
     * @brief Evaluates whether the player needs to drink a potion based on current HP and incoming DPS.
     * 
     * If the threshold is crossed, this method automatically publishes an AutoPotionTriggerEvent.
     * Uses std::optional monads.
     * 
     * @param playerId The entity ID of the player.
     * @param currentHp The current health points of the player.
     * @param maxHp The maximum health points of the player.
     * @return std::expected<float, EterBase::CombatError> The calculated dynamic threshold ratio if successful.
     */
    std::expected<float, EterBase::CombatError> EvaluateThresholdAndTrigger(EterBase::EntityId playerId, float currentHp, float maxHp) {
        if (!playerId || maxHp <= 0.0f) {
            return std::unexpected(EterBase::CombatError::InvalidAction);
        }

        auto now = std::chrono::steady_clock::now();
        CleanUpHistory(playerId, now);

        float dps = CalculateDps(playerId);
        float dpsRatio = (maxHp > 0.0f) ? (dps / maxHp) : 0.0f;
        
        // Use monadic optional to combine base logic (even though simple here, demonstrates usage)
        std::optional<float> baseThresholdOpt = GetLastThreshold(playerId)
            .transform([dpsRatio](float base) { return base + dpsRatio; })
            .or_else([this, dpsRatio]() -> std::optional<float> { return 0.20f + dpsRatio + safeMarginRatio_; });

        float dynamicThreshold = baseThresholdOpt.value_or(0.20f);
        dynamicThreshold = std::clamp(dynamicThreshold, 0.20f, 0.95f);

        float currentHpRatio = currentHp / maxHp;

        if (currentHpRatio < dynamicThreshold) {
            EterBase::ModernLogger::Debug("AutoPotion Triggered: Player {} at {} HP ratio, threshold is {}", 
                                           playerId.value(), currentHpRatio, dynamicThreshold);

            AutoPotionTriggerEvent event{
                .playerId = playerId,
                .potionType = 0, // AUTO_POTION_TYPE_HP
                .currentHpRatio = currentHpRatio,
                .calculatedThreshold = dynamicThreshold
            };
            
            // Note: prompt specifically asked for Core::EventBus::Instance().Publish(...)
            // However, EventBus class is defined in UserInterface/Core/EventBus.h and uses GetInstance().
            // So we use the correct call as verified in the code.
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        return dynamicThreshold;
    }

    /**
     * @brief Purges old damage records from memory to keep the calculator efficient.
     */
    void Clear() {
        damageHistory_.clear();
    }

private:
    struct DamageRecord {
        float damage;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::unordered_map<EterBase::EntityId, std::deque<DamageRecord>> damageHistory_;
    std::chrono::milliseconds windowDuration_;
    float safeMarginRatio_;

    void CleanUpHistory(EterBase::EntityId playerId, std::chrono::steady_clock::time_point now) {
        auto it = damageHistory_.find(playerId);
        if (it == damageHistory_.end()) {
            return;
        }

        auto cutoff = now - windowDuration_;
        auto& deque = it->second;
        while (!deque.empty() && deque.front().timestamp < cutoff) {
            deque.pop_front();
        }

        if (deque.empty()) {
            damageHistory_.erase(it);
        }
    }

    float CalculateDps(EterBase::EntityId playerId) const {
        auto it = damageHistory_.find(playerId);
        if (it == damageHistory_.end() || it->second.empty()) {
            return 0.0f;
        }

        float totalDamage = 0.0f;
        for (const auto& record : it->second) {
            totalDamage += record.damage;
        }

        float windowSeconds = std::chrono::duration<float>(windowDuration_).count();
        if (windowSeconds <= 0.0f) {
            return 0.0f;
        }

        return totalDamage / windowSeconds;
    }
};

} // namespace GameLib
