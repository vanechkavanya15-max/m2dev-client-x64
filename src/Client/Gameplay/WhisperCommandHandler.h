#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "../../Client/Core/DomainCommands.h"
#include "../../EterBase/Result.h"
#include "../../Client/Core/INetworkPort.h"
#include "../../EterBase/ChronoTimer.h"

namespace Client::Gameplay {

/**
 * @brief Zero-Conflict Command Handler for processing Whisper requests.
 * 
 * Enforces recipient name length and anti-spam constraints.
 */
class WhisperCommandHandler {
public:
    WhisperCommandHandler() = default;
    ~WhisperCommandHandler() = default;

    // Non-copyable, non-movable for singleton-like usage in session
    WhisperCommandHandler(const WhisperCommandHandler&) = delete;
    WhisperCommandHandler& operator=(const WhisperCommandHandler&) = delete;
    WhisperCommandHandler(WhisperCommandHandler&&) = delete;
    WhisperCommandHandler& operator=(WhisperCommandHandler&&) = delete;

    /**
     * @brief Executes a Whisper command, sending a message to a specific player.
     * 
     * @param cmd The whisper command payload containing recipient name and message.
     * @param networkPort Shared pointer to the network port for transmission.
     * @param timer Reference to the timer for anti-spam validation.
     * @return Result<void, CommandError> indicating success or domain error.
     */
    [[nodiscard]] EterBase::Result<void, Core::CommandError> Execute(
        const Core::WhisperCommand& cmd, 
        std::shared_ptr<Core::INetworkPort> networkPort, 
        ChronoTimer& timer);

private:
    // Anti-spam cooldown limit in milliseconds
    static constexpr uint32_t WHISPER_COOLDOWN_MS = 500;
    // Max recipient name length
    static constexpr size_t MAX_RECIPIENT_LENGTH = 24;

    std::unordered_map<std::string, uint32_t> m_lastWhisperTimes;
};

} // namespace Client::Gameplay
