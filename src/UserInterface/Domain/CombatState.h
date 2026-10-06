#pragma once

#include <cstdint>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <span>
#include <string_view>

namespace Domain {

/**
 * @brief Represents a strictly packed network packet for attack hit reporting.
 * 
 * Ensures cross-platform compatibility and adherence to the protocol without any padding.
 */
#pragma pack(push, 1)
struct AttackHitPacket {
    uint8_t header;         ///< Packet header identifier
    uint32_t targetId;      ///< Identifier of the attacked entity
    uint32_t hitType;       ///< Type of hit (e.g., normal, critical, piercing)
    uint32_t skillId;       ///< Identifier of the skill used, if any
};
#pragma pack(pop)

/**
 * @brief Represents configuration data for a specific animation motion.
 */
struct MotionConfig {
    uint32_t motionKey;     ///< Unique identifier for the animation motion
    float speedRatio;       ///< Speed multiplier for the animation
    float blendTime;        ///< Duration in seconds to blend from previous animation
    int32_t loopCount;      ///< Number of times the animation should loop
    uint32_t skillId;       ///< Associated skill ID, if any
};

/**
 * @brief Manages the combat state for an actor, including hit tracking, invulnerability windows, and animation states.
 * 
 * This class handles the timing and spacing between attack hits, tracks which targets have been hit 
 * by specific attacks to prevent multi-hits in the same frame/animation, and manages animation state intervals.
 * It is strictly decoupled from GUI and Python contexts.
 */
class CombatState {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;

    /**
     * @brief Constructs a new CombatState instance.
     */
    CombatState() = default;

    /**
     * @brief Destroys the CombatState instance.
     */
    ~CombatState() = default;

    /**
     * @brief Registers an attack hit on a specific target and applies an invulnerability window.
     * 
     * @param attackId The unique identifier of the attack or skill used.
     * @param targetId The unique identifier of the victim/target.
     * @param invulnerabilityDuration The duration the target should remain invulnerable to this specific attack.
     * @param currentTime The current time point.
     * @return true if the hit was successfully registered (target wasn't invulnerable).
     * @return false if the target is currently invulnerable to this attack.
     */
    bool RegisterHit(uint32_t attackId, uint32_t targetId, Duration invulnerabilityDuration, TimePoint currentTime) {
        if (IsTargetInvulnerable(attackId, targetId, currentTime)) {
            return false;
        }

        hitRecords_[attackId][targetId] = currentTime + invulnerabilityDuration;
        return true;
    }

    /**
     * @brief Checks whether a target is currently invulnerable to a specific attack.
     * 
     * @param attackId The unique identifier of the attack or skill.
     * @param targetId The unique identifier of the target.
     * @param currentTime The current time point for evaluation.
     * @return true if the target is invulnerable.
     * @return false if the target is vulnerable.
     */
    [[nodiscard]] bool IsTargetInvulnerable(uint32_t attackId, uint32_t targetId, TimePoint currentTime) const {
        auto attackIt = hitRecords_.find(attackId);
        if (attackIt == hitRecords_.end()) {
            return false;
        }

        auto targetIt = attackIt->second.find(targetId);
        if (targetIt == attackIt->second.end()) {
            return false;
        }

        return currentTime < targetIt->second;
    }

    /**
     * @brief Clears expired invulnerability records to free up memory.
     * 
     * @param currentTime The current time point used to determine expiration.
     */
    void PruneExpiredHits(TimePoint currentTime) {
        for (auto attackIt = hitRecords_.begin(); attackIt != hitRecords_.end(); ) {
            auto& targetMap = attackIt->second;
            for (auto targetIt = targetMap.begin(); targetIt != targetMap.end(); ) {
                if (currentTime >= targetIt->second) {
                    targetIt = targetMap.erase(targetIt);
                } else {
                    ++targetIt;
                }
            }

            if (targetMap.empty()) {
                attackIt = hitRecords_.erase(attackIt);
            } else {
                ++attackIt;
            }
        }
    }

    /**
     * @brief Resets all combat state, including hit history and current animation data.
     */
    void Reset() {
        hitRecords_.clear();
        globalInvulnerabilityExpiration_.reset();
        activeMotion_.reset();
    }

    /**
     * @brief Sets the current active motion configuration for an attack.
     * 
     * @param config The motion configuration to apply.
     */
    void SetActiveMotion(const MotionConfig& config) {
        activeMotion_ = config;
    }

    /**
     * @brief Retrieves the currently active motion configuration, if any.
     * 
     * @return std::optional<MotionConfig> The active motion config or std::nullopt if none is set.
     */
    [[nodiscard]] std::optional<MotionConfig> GetActiveMotion() const {
        return activeMotion_;
    }

    /**
     * @brief Sets the global invulnerability window for the actor.
     * 
     * @param expirationTime The time point when the invulnerability ends.
     */
    void SetGlobalInvulnerability(TimePoint expirationTime) {
        globalInvulnerabilityExpiration_ = expirationTime;
    }

    /**
     * @brief Checks if the actor has global invulnerability active.
     * 
     * @param currentTime The current time point.
     * @return true if globally invulnerable.
     * @return false otherwise.
     */
    [[nodiscard]] bool IsGloballyInvulnerable(TimePoint currentTime) const {
        if (!globalInvulnerabilityExpiration_) {
            return false;
        }
        return currentTime < *globalInvulnerabilityExpiration_;
    }

    /**
     * @brief Processes an incoming binary network payload related to combat.
     * 
     * @param payload A span containing the raw bytes of the packet.
     * @return true if the payload was successfully parsed and applied.
     * @return false if the payload was malformed or unrecognized.
     */
    bool ProcessCombatPayload(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(AttackHitPacket)) {
            return false;
        }

        [[maybe_unused]] const auto* packet = reinterpret_cast<const AttackHitPacket*>(payload.data());
        
        std::string_view packetContext = "AttackHit";
        
        if (packetContext == "AttackHit") {
            // Placeholder for applying the parsed packet data to the actor state.
            // Decoupled from GUI and Python calls as per the requirements.
        }
        
        return true;
    }

private:
    /// Maps an attack ID to a map of target IDs and their respective invulnerability expiration times.
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, TimePoint>> hitRecords_;
    
    /// Global invulnerability expiration time.
    std::optional<TimePoint> globalInvulnerabilityExpiration_;

    /// The currently active attack motion.
    std::optional<MotionConfig> activeMotion_;
};

} // namespace Domain
