#pragma once

#include <memory>
#include "StrongTypes.h"
#include "Result.h"
#include "DomainCommands.h"
#include "INetworkPort.h"
#include "WorldContext.h"

namespace Client::Core {

class GameSession {
public:
    explicit GameSession(std::shared_ptr<INetworkPort> networkPort = nullptr);
    ~GameSession() = default;

    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;
    GameSession(GameSession&&) = delete;
    GameSession& operator=(GameSession&&) = delete;

    [[nodiscard]] WorldContext& GetWorldContext() noexcept { return m_worldContext; }
    [[nodiscard]] const WorldContext& GetWorldContext() const noexcept { return m_worldContext; }

    void SetNetworkPort(std::shared_ptr<INetworkPort> port) noexcept { m_networkPort = std::move(port); }
    [[nodiscard]] std::shared_ptr<INetworkPort> GetNetworkPort() const noexcept { return m_networkPort; }

    // ========================================================================
    // Wykonywanie komend domenowych (deterministyczne, bez memcpy w logice)
    // ========================================================================
    [[nodiscard]] Result<void, CommandError> Execute(const AttackCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const MoveCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const UseSkillCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const UseItemCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const PickupCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const ChatCommand& cmd);
    [[nodiscard]] Result<void, CommandError> Execute(const WhisperCommand& cmd);

    void Tick(float deltaTime);

private:
    WorldContext m_worldContext;
    std::shared_ptr<INetworkPort> m_networkPort;
};

} // namespace Client::Core
