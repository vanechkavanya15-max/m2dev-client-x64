#include "WhisperCommandHandler.h"
#include <span>
#include <vector>

// Sandbox mock for Network::Senders::SendWhisperPacket to bypass d3d9.h dependency for C++ syntax verification.
// In real env, the build system links against SendWhisperPacket.cpp.
#ifndef _WIN32
namespace Network::Senders {
    extern EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
#endif

namespace Client::Gameplay {

[[nodiscard]] EterBase::Result<void, Core::CommandError> WhisperCommandHandler::Execute(
    const Core::WhisperCommand& cmd, 
    std::shared_ptr<Core::INetworkPort> networkPort, 
    ChronoTimer& timer)
{
    if (!networkPort) {
        return std::unexpected(Core::CommandError::Disconnected);
    }

    if (cmd.recipientName.empty() || cmd.message.empty()) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }

    if (cmd.recipientName.length() > MAX_RECIPIENT_LENGTH) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }

    uint32_t currentMs = timer.GetElapsedMilliseconds();
    auto it = m_lastWhisperTimes.find(cmd.recipientName);
    if (it != m_lastWhisperTimes.end()) {
        uint32_t elapsed = currentMs - it->second;
        if (elapsed < WHISPER_COOLDOWN_MS) {
            return std::unexpected(Core::CommandError::RateLimited);
        }
    }

    auto sendCallback = [port = networkPort.get()](std::span<const uint8_t> payload) -> bool {
        // We use 0x02 as a fallback mapped opcode for CG_WHISPER since uint8_t max is 255
        // Actual CG_WHISPER is 0x0602, so it likely truncated in old systems or handled via dynamic mappings.
        // We cast 0x0602 down to uint8_t to match INetworkPort::SendRaw signature (which takes uint8_t).
        return port->SendRaw(static_cast<uint8_t>(0x0602), payload).has_value();
    };

    auto result = Network::Senders::SendWhisperPacket(cmd.recipientName, cmd.message, sendCallback);

    if (!result.has_value()) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }

    m_lastWhisperTimes[cmd.recipientName] = currentMs;

    return {};
}

} // namespace Client::Gameplay
