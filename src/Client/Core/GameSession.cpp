#include "GameSession.h"

namespace Client::Core {

GameSession::GameSession(std::shared_ptr<INetworkPort> networkPort)
    : m_networkPort(std::move(networkPort))
{
}

Result<void, CommandError> GameSession::Execute(const AttackCommand& cmd) {
    if (cmd.targetVid.get() == 0) {
        return std::unexpected(CommandError::InvalidTarget);
    }
    if (m_worldContext.isDead) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    // Wykonanie komendy ataku – w fazie 1 walidacja lokalna, wysylka portem
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const MoveCommand& cmd) {
    if (m_worldContext.isDead) {
        return std::unexpected(CommandError::MovementBlocked);
    }
    m_worldContext.localPlayerCoords = cmd.destination;
    m_worldContext.localPlayerRotation = cmd.rotation;

    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const UseSkillCommand& cmd) {
    if (cmd.skillId.get() == 0) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_worldContext.isDead) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const UseItemCommand& cmd) {
    if (cmd.slot.get() == 0xFFFF) {
        return std::unexpected(CommandError::SlotEmpty);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const PickupCommand& cmd) {
    if (cmd.itemVid.get() == 0) {
        return std::unexpected(CommandError::InvalidTarget);
    }
    if (m_worldContext.isDead) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const ChatCommand& cmd) {
    if (cmd.message.empty()) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(CommandError::Disconnected);
    }
    return {};
}

void GameSession::Tick(float /*deltaTime*/) {
    // Deterministyczna aktualizacja lokalnego stanu sesji
}

} // namespace Client::Core
