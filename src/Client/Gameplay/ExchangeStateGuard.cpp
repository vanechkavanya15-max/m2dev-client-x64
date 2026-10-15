#include "ExchangeStateGuard.h"

namespace Client::Gameplay {

Client::Core::Result<void, Client::Core::CommandError> ExchangeStateGuard::Validate(
    const PlayerExchange& exchange,
    EterBase::EntityId initiator,
    EterBase::EntityId target) const
{
    const auto* initiator_participant = exchange.GetParticipant(initiator);
    const auto* target_participant = exchange.GetParticipant(target);

    if (!initiator_participant || !target_participant) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (initiator_participant->is_locked || target_participant->is_locked) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    return {};
}

} // namespace Client::Gameplay
