#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <optional>
#include <span>

namespace Metin2::Domain {

#pragma pack(push, 1)
/**
 * @brief Represents the network packet structure for a skill activation request.
 * Must be strictly aligned for network transmission.
 */
struct SkillActivationPacket {
    uint8_t header;
    uint32_t skillId;
    uint32_t targetId;
};
#pragma pack(pop)

/**
 * @brief Represents a single active skill's state.
 */
struct ActiveSkillState {
    uint32_t skillId;
    float currentCooldown;
    float maxCooldown;
    bool isActive;
};

/**
 * @brief Manages the state of active skills for a player.
 * Ensures Single Responsibility Principle by only tracking skill states
 * and completely decoupling from GUI interactions.
 */
class SkillRegistry {
public:
    SkillRegistry() = default;
    ~SkillRegistry() = default;

    // Prevent copying and assignment
    SkillRegistry(const SkillRegistry&) = delete;
    SkillRegistry& operator=(const SkillRegistry&) = delete;

    /**
     * @brief Updates the cooldown for a specific skill.
     * @param skillId The ID of the skill to update.
     * @param delta The time passed since the last update.
     */
    void UpdateSkillCooldown(uint32_t skillId, float delta) {
        for (auto& state : activeSkills_) {
            if (state.skillId == skillId) {
                if (state.currentCooldown > 0.0f) {
                    state.currentCooldown -= delta;
                    if (state.currentCooldown < 0.0f) {
                        state.currentCooldown = 0.0f;
                    }
                }
                break;
            }
        }
    }

    /**
     * @brief Activates a skill, resetting its cooldown.
     * @param skillId The ID of the skill to activate.
     * @param maxCooldown The maximum cooldown of the skill.
     */
    void ActivateSkill(uint32_t skillId, float maxCooldown) {
        for (auto& state : activeSkills_) {
            if (state.skillId == skillId) {
                state.currentCooldown = maxCooldown;
                state.maxCooldown = maxCooldown;
                state.isActive = true;
                return;
            }
        }
        // If skill doesn't exist in registry, add it
        activeSkills_.push_back({skillId, maxCooldown, maxCooldown, true});
    }

    /**
     * @brief Retrieves the state of a specific skill.
     * @param skillId The ID of the skill to retrieve.
     * @return std::optional containing the skill state if found, std::nullopt otherwise.
     */
    std::optional<ActiveSkillState> GetSkillState(uint32_t skillId) const {
        for (const auto& state : activeSkills_) {
            if (state.skillId == skillId) {
                return state;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Clears all active skills from the registry.
     */
    void Clear() {
        activeSkills_.clear();
    }

    /**
     * @brief Processes a network packet for skill activation.
     * @param buffer A span containing the raw network packet data.
     * @return true if the packet was successfully parsed and applied, false otherwise.
     */
    bool ProcessSkillPacket(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(SkillActivationPacket)) {
            return false;
        }

        const auto* packet = reinterpret_cast<const SkillActivationPacket*>(buffer.data());
        
        // Basic validation could go here
        
        // This is a placeholder for actual logic which might involve 
        // activating a skill based on the packet data and the skill's defined cooldown.
        // For demonstration, we assume a default max cooldown of 10.0f for packet activations.
        ActivateSkill(packet->skillId, 10.0f);
        
        return true;
    }

private:
    std::vector<ActiveSkillState> activeSkills_;
};

} // namespace Metin2::Domain
