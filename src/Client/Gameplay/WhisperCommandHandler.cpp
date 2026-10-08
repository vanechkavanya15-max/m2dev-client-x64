#include "EterBase/StdAfx.h"
#include "WhisperCommandHandler.h"
#include "../Network/SocialCommandEncoder.h"

namespace Client::Gameplay {

EterBase::Result<void, Core::CommandError> WhisperCommandHandler::Execute(
    const Core::WhisperCommand& cmd, 
    std::shared_ptr<Core::INetworkPort> networkPort, 
    ChronoTimer& timer)
{
    if (!networkPort || !networkPort->IsConnected()) {
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

    Client::Network::WhisperCommand whisperCmd{
        .targetName = cmd.recipientName,
        .message = cmd.message
    };
    auto encodeRes = Client::Network::SocialCommandEncoder::EncodeWhisper(whisperCmd);
    if (!encodeRes) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }

    auto sendRes = networkPort->SendRaw(static_cast<uint8_t>(0x02), encodeRes.value());
    if (!sendRes) {
        return std::unexpected(Core::CommandError::Disconnected);
    }

    m_lastWhisperTimes[cmd.recipientName] = currentMs;
    return {};
}

} // namespace Client::Gameplay
