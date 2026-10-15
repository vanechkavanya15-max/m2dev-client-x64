#include "EterBase/StdAfx.h"
#include "MovementStepValidator.h"

namespace Client::Gameplay {

EterBase::Result<bool, Client::Core::CommandError> MovementStepValidator::ValidateStep(
    const Client::Core::MapCoords& current, 
    const Client::Core::MapCoords& next, 
    float dt, 
    float speed) noexcept {

    if (dt <= 0.0f) {
        if (current == next) {
            return true;
        }
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (speed < 0.0f) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    const float distance = current.Distance(next);
    // Include small epsilon to account for float inaccuracy
    const float max_allowed_distance = speed * dt + 0.01f;

    if (distance > max_allowed_distance) {
        return std::unexpected(Client::Core::CommandError::MovementBlocked);
    }

    return true;
}

} // namespace Client::Gameplay
