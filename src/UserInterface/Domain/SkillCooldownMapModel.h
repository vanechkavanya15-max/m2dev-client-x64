#pragma once

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

#include <chrono>
#include <unordered_map>
#include <optional>
#include <algorithm>

namespace UserInterface::Domain {

/**
 * @brief Event published when a skill's cooldown state changes.
 * 
 * Allows decoupled UI or other systems to react to cooldown updates.
 */
struct SkillCooldownEvent : public Core::IEvent {
    EterBase::SkillId skillId;
    std::chrono::steady_clock::time_point expirationTime;
    std::chrono::milliseconds maxDuration;

    /**
     * @brief Constructs a new Skill Cooldown Event.
     * @param skillId The ID of the skill.
     * @param expirationTime The absolute time when the cooldown expires.
     * @param maxDuration The total duration of the cooldown.
     */
    SkillCooldownEvent(EterBase::SkillId skillId, std::chrono::steady_clock::time_point expirationTime, std::chrono::milliseconds maxDuration)
        : skillId(skillId), expirationTime(expirationTime), maxDuration(maxDuration) {}
};

/**
 * @brief Stores the cooldown state for a single skill.
 */
struct SkillCooldownData {
    std::chrono::steady_clock::time_point expirationTime; ///< The time when the cooldown will finish.
    std::chrono::milliseconds maxDuration;                ///< The maximum duration of the cooldown.
};

/**
 * @brief Manages the cooldowns for skills in memory.
 * 
 * This model is fully decoupled from the GUI and updates only memory state.
 * It publishes `SkillCooldownEvent` via the `EventBus` to notify observers.
 */
class SkillCooldownMapModel {
public:
    SkillCooldownMapModel() = default;

    /**
     * @brief Sets or updates the cooldown for a specific skill.
     * 
     * @param skillId The ID of the skill.
     * @param duration The duration of the cooldown.
     * @return Result<void, EterBase::CombatError> Success or an error if invalid parameters are provided.
     */
    EterBase::Result<void, EterBase::CombatError> SetCooldown(EterBase::SkillId skillId, std::chrono::milliseconds duration) {
        if (duration <= std::chrono::milliseconds::zero()) {
            EterBase::ModernLogger::Warn("Failed to set cooldown for skill {}: duration must be positive.", skillId.value());
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }

        auto expirationTime = std::chrono::steady_clock::now() + duration;
        cooldownMap[skillId] = SkillCooldownData{expirationTime, duration};

        EterBase::ModernLogger::Debug("Set cooldown for skill {} for {} ms.", skillId.value(), duration.count());
        Core::EventBus::GetInstance().Publish(SkillCooldownEvent{skillId, expirationTime, duration});

        return {};
    }

    /**
     * @brief Retrieves the raw cooldown data for a skill, if it is on cooldown.
     * 
     * @param skillId The ID of the skill to query.
     * @return std::optional<SkillCooldownData> The cooldown data, or std::nullopt if not on cooldown or expired.
     */
    [[nodiscard]] std::optional<SkillCooldownData> GetCooldown(EterBase::SkillId skillId) const {
        auto it = cooldownMap.find(skillId);
        if (it != cooldownMap.end()) {
            if (it->second.expirationTime > std::chrono::steady_clock::now()) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Calculates the remaining cooldown time for a skill.
     * 
     * @param skillId The ID of the skill.
     * @return std::optional<std::chrono::milliseconds> The remaining time, or std::nullopt if not on cooldown.
     */
    [[nodiscard]] std::optional<std::chrono::milliseconds> GetRemainingCooldown(EterBase::SkillId skillId) const {
        return GetCooldown(skillId).transform([](const SkillCooldownData& data) {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(data.expirationTime - now);
        });
    }

    /**
     * @brief Calculates the progress of the cooldown for a skill.
     * 
     * @param skillId The ID of the skill.
     * @return std::optional<float> The progress (0.0 to 1.0) where 1.0 means fully ready, or std::nullopt if not on cooldown.
     */
    [[nodiscard]] std::optional<float> GetCooldownProgress(EterBase::SkillId skillId) const {
        return GetCooldown(skillId).transform([](const SkillCooldownData& data) -> float {
            auto now = std::chrono::steady_clock::now();
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(data.expirationTime - now).count();
            auto max = data.maxDuration.count();
            
            if (max <= 0) return 1.0f; // Prevent division by zero, should not happen due to SetCooldown check
            
            float progress = 1.0f - (static_cast<float>(remaining) / static_cast<float>(max));
            return std::clamp(progress, 0.0f, 1.0f);
        });
    }

    /**
     * @brief Clears the cooldown for a specific skill.
     * 
     * @param skillId The ID of the skill to clear.
     */
    void ClearCooldown(EterBase::SkillId skillId) {
        if (cooldownMap.erase(skillId) > 0) {
            EterBase::ModernLogger::Debug("Cleared cooldown for skill {}.", skillId.value());
            Core::EventBus::GetInstance().Publish(SkillCooldownEvent{
                skillId, 
                std::chrono::steady_clock::now(), 
                std::chrono::milliseconds::zero()
            });
        }
    }

    /**
     * @brief Clears all skill cooldowns.
     */
    void ClearAll() {
        for (const auto& [skillId, _] : cooldownMap) {
            Core::EventBus::GetInstance().Publish(SkillCooldownEvent{
                skillId, 
                std::chrono::steady_clock::now(), 
                std::chrono::milliseconds::zero()
            });
        }
        cooldownMap.clear();
        EterBase::ModernLogger::Debug("Cleared all skill cooldowns.");
    }

private:
    std::unordered_map<EterBase::SkillId, SkillCooldownData> cooldownMap; ///< Map of skill IDs to their cooldown data.
};

} // namespace UserInterface::Domain
