#include "CombatCommandHandler.h"

namespace Client::Gameplay {

CombatCommandHandler::CombatCommandHandler(const ICombatEntityQuery& entityQuery)
    : m_entityQuery(entityQuery) {
}

Core::Result<void, Core::CommandError> CombatCommandHandler::HandleAttack(
    const Core::AttackCommand& cmd,
    const Core::WorldContext& context) const {
    
    // 1. Weryfikacja czy gracz nie jest martwy
    if (context.isDead) {
        return std::unexpected(Core::CommandError::MovementBlocked);
    }

    // 2. Walidacja wartosci ID celu
    if (cmd.targetVid.get() == 0) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 3. Weryfikacja czy celem nie jest sam gracz
    if (cmd.targetVid == context.localPlayerVid) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 4. Weryfikacja czy cel w ogole istnieje w swiecie
    if (!m_entityQuery.IsValidEntity(cmd.targetVid)) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // 5. Weryfikacja czy cel jest zywy
    if (m_entityQuery.IsEntityDead(cmd.targetVid)) {
        return std::unexpected(Core::CommandError::InvalidTarget);
    }

    // Jezeli wszystkie walidacje przeszly pomyslnie, zwracamy sukces
    return {};
}

} // namespace Client::Gameplay
