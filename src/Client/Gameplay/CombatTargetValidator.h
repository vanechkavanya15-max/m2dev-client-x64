#pragma once

#include "../Core/Result.h"
#include "../Core/DomainCommands.h"
#include "../Core/StrongTypes.h"
#include <cstdint>

namespace Client::Gameplay {

enum class EntityType : uint8_t {
    Unknown = 0,
    Player = 1,
    Monster = 2,
    NPC = 3,
    Pet = 4,
    Mount = 5
};

struct CombatEntityState {
    Core::EntityVid vid{0};
    Core::RaceVnum race{0};
    EntityType type{EntityType::Unknown};
    uint8_t level{1};
    uint8_t empire{0};
    bool isInvincible{false};
    bool inSafeZone{false};
    bool isDuelActive{false};
    bool isPvPModeActive{false};
};

class CombatTargetValidator {
public:
    [[nodiscard]] static Core::Result<bool, Core::CommandError> ValidateTarget(
        const CombatEntityState& attacker,
        const CombatEntityState& target) noexcept;
};

} // namespace Client::Gameplay
