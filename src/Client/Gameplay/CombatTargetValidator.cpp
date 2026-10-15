#include "CombatTargetValidator.h"

namespace Client::Gameplay {

[[nodiscard]] Core::Result<bool, Core::CommandError> CombatTargetValidator::ValidateTarget(
    const CombatEntityState& attacker,
    const CombatEntityState& target) noexcept {

    // 1. Cannot attack self
    if (attacker.vid == target.vid) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 2. Safe zone checks
    if (attacker.inSafeZone || target.inSafeZone) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 3. Invincibility check
    if (target.isInvincible) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 4. Cannot attack NPCs (and Shops, which are treated as NPCs)
    if (target.type == EntityType::NPC) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 5. Player vs Player specific rules
    if (attacker.type == EntityType::Player && target.type == EntityType::Player) {
        // Player Protection: Cannot engage in PvP if either is under level 15
        if (attacker.level < 15 || target.level < 15) {
            return std::unexpected(Core::CommandError::InvalidTarget);
        }

        // Duel takes precedence
        if (attacker.isDuelActive && target.isDuelActive) {
            return true;
        }

        // If PvP mode is active, you can attack other empires, 
        // or same empire if explicitly flagged (Free mode, etc., handled here via isPvPModeActive for simplicity)
        // A full implementation would check PvP modes (Peace, Revenge, Guild, Free).
        // For this validator, we allow if different empires, OR if PvP mode is active.
        if (attacker.empire == target.empire && !attacker.isPvPModeActive) {
            return std::unexpected(Core::CommandError::InvalidTarget);
        }
    }

    // Attack allowed (Monsters, valid Players, etc.)
    return true;
}

} // namespace Client::Gameplay
