#pragma once

#include <cstdint>
#include <optional>
#include "../Core/Result.h"
#include "../Core/DomainErrors.h"
#include "../../UserInterface/Domain/AffectFlagsHelper.h"
#include "CombatDomain.h"

namespace Client::Gameplay {

    /**
     * @brief Context under which a skill is cast.
     */
    struct CastContext {
        uint32_t currentSP{0};
        domain::AffectFlagsData affectFlags{};
        std::optional<WeaponType> currentWeapon{std::nullopt};
    };

    /**
     * @brief Requirements that a skill has in order to be cast.
     */
    struct SkillRequirements {
        uint32_t spCost{0};
        bool requiresWeapon{false};
        uint32_t allowedWeaponsMask{0xFFFFFFFF}; // Bitmask of WeaponType (1 << static_cast<uint32_t>(WeaponType))
    };

    /**
     * @class SkillCastValidator
     * @brief Stateless validator checking if a player is allowed to cast a given skill.
     * 
     * Verifies SP, character states like Stun, and weapon requirements.
     */
    class SkillCastValidator {
    public:
        [[nodiscard]] static constexpr Core::Result<void, Core::SkillError> ValidateCast(
            const CastContext& context, 
            const SkillRequirements& reqs) noexcept 
        {
            // 1. Check for inhibiting states like Stun
            if (domain::AffectFlagsHelper::isStunned(context.affectFlags)) {
                return std::unexpected(Core::SkillError::RequirementNotMet);
            }

            // 2. Check for sufficient SP
            if (context.currentSP < reqs.spCost) {
                return std::unexpected(Core::SkillError::NotEnoughSP);
            }

            // 3. Check for weapon requirements if applicable
            if (reqs.requiresWeapon) {
                if (!context.currentWeapon.has_value()) {
                    return std::unexpected(Core::SkillError::RequirementNotMet);
                }

                const uint32_t weaponBit = 1u << static_cast<uint32_t>(*context.currentWeapon);
                if ((reqs.allowedWeaponsMask & weaponBit) == 0) {
                    return std::unexpected(Core::SkillError::RequirementNotMet);
                }
            }

            return {};
        }
    };

} // namespace Client::Gameplay
