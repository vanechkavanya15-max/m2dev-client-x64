#include "CombatRangeCalculator.h"

namespace Client::Gameplay {

float CombatRangeCalculator::GetBaseRange(WeaponType weaponType) noexcept {
    switch (weaponType) {
        case WeaponType::Sword:
            return 200.0f;
        case WeaponType::TwoHanded:
            return 300.0f;
        case WeaponType::Bow:
            return 2500.0f;
        case WeaponType::Dagger:
            return 150.0f;
        default:
            return 200.0f; // Default fallback for None, Fan, Bell etc if not specified
    }
}

Core::Result<bool, Core::CommandError> CombatRangeCalculator::IsTargetInRange(
    const Core::MapCoords& attackerPos,
    const Core::MapCoords& targetPos,
    WeaponType weaponType
) noexcept {
    float distance = attackerPos.Distance(targetPos);
    float maxRange = GetBaseRange(weaponType);
    
    if (distance > maxRange) {
        return std::unexpected(Core::CommandError::OutOfRange);
    }
    
    return true;
}

} // namespace Client::Gameplay
