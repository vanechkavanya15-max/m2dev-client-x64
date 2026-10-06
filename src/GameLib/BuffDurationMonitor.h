#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <string_view>

namespace Metin2::CombatMath {

/**
 * @brief Represents the types of buffs that can be monitored.
 */
enum class BuffType : uint8_t {
    None = 0,
    AuraMiecza = 1,
    SilneCialo = 2,
    Berek = 3
};

/**
 * @brief Gets the string representation of a BuffType.
 * @param type The type of the buff.
 * @return A string view representing the name of the buff.
 */
constexpr std::string_view GetBuffName(BuffType type) {
    switch (type) {
        case BuffType::AuraMiecza: return "Aura Miecza";
        case BuffType::SilneCialo: return "Silne Cialo";
        case BuffType::Berek:      return "Berek";
        default:                   return "Unknown";
    }
}

/**
 * @brief Represents a single buff currently active on the player.
 */
struct ActiveBuff {
    BuffType type;             ///< The type of the buff.
    uint32_t remainingTimeMs;  ///< Remaining duration of the buff in milliseconds.
    bool warningTriggered;     ///< Whether the warning threshold has already been triggered.
};

/**
 * @brief Represents the result of updating the buff monitor, containing notifications for expiring buffs.
 */
struct BuffUpdateResult {
    bool hasExpiringBuffs;                 ///< True if any buffs are expiring soon.
    std::vector<BuffType> expiringBuffs;   ///< List of buffs that crossed the warning threshold this frame.
};

/**
 * @brief Monitors active buffs and tracks their remaining duration, triggering warnings when they are about to expire.
 */
class BuffDurationMonitor {
public:
    /**
     * @brief Constructs a new BuffDurationMonitor with a specified warning threshold.
     * @param warningThresholdMs Time in milliseconds before expiration to trigger a warning.
     */
    explicit BuffDurationMonitor(uint32_t warningThresholdMs = 5000)
        : warningThresholdMs(warningThresholdMs) {}

    /**
     * @brief Adds or refreshes a buff with a specified duration.
     * @param type The type of the buff to add or refresh.
     * @param durationMs The total duration of the buff in milliseconds.
     * @return True if the buff was successfully added or refreshed, false if the type is None.
     */
    bool AddBuff(BuffType type, uint32_t durationMs) {
        if (type == BuffType::None) {
            return false;
        }

        for (auto& buff : activeBuffs) {
            if (buff.type == type) {
                buff.remainingTimeMs = durationMs;
                buff.warningTriggered = (durationMs <= warningThresholdMs);
                return true;
            }
        }

        activeBuffs.push_back({type, durationMs, durationMs <= warningThresholdMs});
        return true;
    }

    /**
     * @brief Removes a buff from the monitor.
     * @param type The type of the buff to remove.
     * @return True if the buff was found and removed, false otherwise.
     */
    bool RemoveBuff(BuffType type) {
        for (auto it = activeBuffs.begin(); it != activeBuffs.end(); ++it) {
            if (it->type == type) {
                activeBuffs.erase(it);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Updates the active buffs by decreasing their remaining times.
     * @param deltaTimeMs Time elapsed since the last update in milliseconds.
     * @return A BuffUpdateResult containing any buffs that just crossed the warning threshold.
     */
    [[nodiscard]] BuffUpdateResult Update(uint32_t deltaTimeMs) {
        BuffUpdateResult result{};
        result.hasExpiringBuffs = false;

        for (auto it = activeBuffs.begin(); it != activeBuffs.end(); ) {
            if (it->remainingTimeMs <= deltaTimeMs) {
                it->remainingTimeMs = 0;
                // Buff expired, it should be removed.
                it = activeBuffs.erase(it);
            } else {
                it->remainingTimeMs -= deltaTimeMs;

                if (!it->warningTriggered && it->remainingTimeMs <= warningThresholdMs) {
                    it->warningTriggered = true;
                    result.hasExpiringBuffs = true;
                    result.expiringBuffs.push_back(it->type);
                }
                ++it;
            }
        }

        return result;
    }

    /**
     * @brief Checks if a specific buff is currently active.
     * @param type The type of the buff to check.
     * @return True if the buff is active, false otherwise.
     */
    [[nodiscard]] bool IsBuffActive(BuffType type) const {
        for (const auto& buff : activeBuffs) {
            if (buff.type == type) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Gets the remaining duration of a specific active buff.
     * @param type The type of the buff to query.
     * @return The remaining time in milliseconds if active, or std::nullopt if the buff is not active.
     */
    [[nodiscard]] std::optional<uint32_t> GetRemainingTime(BuffType type) const {
        for (const auto& buff : activeBuffs) {
            if (buff.type == type) {
                return buff.remainingTimeMs;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Clears all active buffs.
     */
    void ClearAllBuffs() {
        activeBuffs.clear();
    }

    /**
     * @brief Sets the warning threshold.
     * @param thresholdMs The new warning threshold in milliseconds.
     */
    void SetWarningThreshold(uint32_t thresholdMs) {
        warningThresholdMs = thresholdMs;
        // Re-evaluate warning triggers for active buffs
        for (auto& buff : activeBuffs) {
            buff.warningTriggered = (buff.remainingTimeMs <= warningThresholdMs);
        }
    }

    /**
     * @brief Gets the current warning threshold.
     * @return The warning threshold in milliseconds.
     */
    [[nodiscard]] uint32_t GetWarningThreshold() const {
        return warningThresholdMs;
    }

private:
    uint32_t warningThresholdMs;           ///< Time in milliseconds before expiration to trigger a warning.
    std::vector<ActiveBuff> activeBuffs;   ///< List of currently tracked buffs.
};

} // namespace Metin2::CombatMath
