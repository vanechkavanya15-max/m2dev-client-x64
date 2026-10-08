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
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        uint32_t targetVid = cmd.targetVid.get();
        auto sendRes = m_networkPort->SendRaw(1, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&targetVid), sizeof(targetVid)));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const MoveCommand& cmd) {
    if (m_worldContext.isDead) {
        return std::unexpected(CommandError::MovementBlocked);
    }
    m_worldContext.localPlayerCoords = cmd.destination;
    m_worldContext.localPlayerRotation = cmd.rotation;
    m_worldContext.posX = cmd.destination.x;
    m_worldContext.posY = cmd.destination.y;
    m_worldContext.posZ = cmd.destination.z;

    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        auto sendRes = m_networkPort->SendRaw(2, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&cmd.destination), sizeof(cmd.destination)));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
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
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        uint32_t sid = cmd.skillId.get();
        auto sendRes = m_networkPort->SendRaw(3, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&sid), sizeof(sid)));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const UseItemCommand& cmd) {
    if (cmd.slot.get() == 0xFFFF) {
        return std::unexpected(CommandError::SlotEmpty);
    }
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        uint16_t slot = cmd.slot.get();
        auto sendRes = m_networkPort->SendRaw(4, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&slot), sizeof(slot)));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
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
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        uint32_t vid = cmd.itemVid.get();
        auto sendRes = m_networkPort->SendRaw(5, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&vid), sizeof(vid)));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const ChatCommand& cmd) {
    if (cmd.message.empty()) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        auto sendRes = m_networkPort->SendRaw(6, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(cmd.message.data()), cmd.message.size()));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
    }
    return {};
}

Result<void, CommandError> GameSession::Execute(const WhisperCommand& cmd) {
    if (cmd.recipientName.empty() || cmd.message.empty()) {
        return std::unexpected(CommandError::InvalidParameter);
    }
    if (m_networkPort) {
        if (!m_networkPort->IsConnected()) {
            return std::unexpected(CommandError::Disconnected);
        }
        std::string payload = cmd.recipientName + " " + cmd.message;
        auto sendRes = m_networkPort->SendRaw(7, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));
        if (!sendRes.has_value()) {
            return std::unexpected(CommandError::Disconnected);
        }
    }
    return {};
}

void GameSession::Tick(float /*deltaTime*/) {
    // Deterministyczna aktualizacja lokalnego stanu sesji
}

} // namespace Client::Core
