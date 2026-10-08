#include "MovementCommandHandler.h"

namespace Client::Gameplay {

Client::Core::Result<void, Client::Core::CommandError> MovementCommandHandler::Handle(const Client::Core::MoveCommand& cmd) {
    if (m_context.isDead || (m_isStunnedProvider && m_isStunnedProvider())) {
        return std::unexpected(Client::Core::CommandError::MovementBlocked);
    }

    if (cmd.moveType != 0 && cmd.moveType != 1) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    m_context.localPlayerCoords = cmd.destination;
    m_context.localPlayerRotation = cmd.rotation;

    return {};
}

} // namespace Client::Gameplay
