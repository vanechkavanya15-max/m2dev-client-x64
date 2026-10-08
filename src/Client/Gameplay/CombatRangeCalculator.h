#pragma once

#include "../../EterBase/Result.h"
#include "../../Client/Core/Result.h"
#include "../../Client/Core/StrongTypes.h"
#include "../../Client/Core/DomainCommands.h"

namespace Client::Gameplay {

class CombatRangeCalculator {
public:
    enum class WeaponType : uint8_t {
        Sword,
        Dagger,
        Bow,
        TwoHanded,
        Bell,
        Fan,
        Arrow,
        None
    };

    static Core::Result<bool, Core::CommandError> IsTargetInRange(
        const Core::MapCoords& attackerPos,
        const Core::MapCoords& targetPos,
        WeaponType weaponType
    ) noexcept;
    
    static float GetBaseRange(WeaponType weaponType) noexcept;
};

} // namespace Client::Gameplay
