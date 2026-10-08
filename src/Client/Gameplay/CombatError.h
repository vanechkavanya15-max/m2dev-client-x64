#pragma once

#include <cstdint>
#include <string_view>

namespace Client::Gameplay {

enum class CombatError : uint8_t {
    TargetDead,
    TargetOutOfRange,
    CharacterStunned,
    AttackCooldown,
    InvalidTarget
};

[[nodiscard]] constexpr std::string_view ToString(CombatError error) noexcept {
    switch (error) {
        case CombatError::TargetDead: return "Target is dead";
        case CombatError::TargetOutOfRange: return "Target is out of range";
        case CombatError::CharacterStunned: return "Character is stunned";
        case CombatError::AttackCooldown: return "Attack cooldown is active";
        case CombatError::InvalidTarget: return "Invalid target";
    }
    return "Unknown error";
}

} // namespace Client::Gameplay
