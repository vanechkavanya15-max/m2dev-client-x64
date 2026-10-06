#include "StdAfx.h"
#include "SendWhisperPacket.h"
#include <vector>
#include <cstring>
#include <algorithm>

namespace Network::Senders
{
    EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (message.length() >= 255)
        {
            // By legacy behaviour, silently ignoring when > 255 (returns true).
            // However, modernization calls for explicit error feedback.
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (targetName.empty() || targetName.length() > CHARACTER_NAME_MAX_LEN)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const uint16_t messageLength = static_cast<uint16_t>(message.length() + 1);

        TPacketCGWhisper whisperPacket;
        std::memset(&whisperPacket, 0, sizeof(whisperPacket));
        whisperPacket.header = CG::WHISPER;
        whisperPacket.length = static_cast<uint16_t>(sizeof(whisperPacket) + messageLength);

        // Safely copy target name - string_view might not be null-terminated
        std::memcpy(whisperPacket.szNameTo, targetName.data(), targetName.length());

        // Create a contiguous buffer for packet struct and the string payload
        std::vector<uint8_t> buffer;
        buffer.reserve(whisperPacket.length);

        const uint8_t* packetPtr = reinterpret_cast<const uint8_t*>(&whisperPacket);
        buffer.insert(buffer.end(), packetPtr, packetPtr + sizeof(whisperPacket));
        
        const uint8_t* messagePtr = reinterpret_cast<const uint8_t*>(message.data());
        buffer.insert(buffer.end(), messagePtr, messagePtr + message.length());
        buffer.push_back('\0'); // null-terminator for the message

        std::span<const uint8_t> packetSpan(buffer.data(), buffer.size());

        if (!sendCallback(packetSpan))
        {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        return {};
    }
}
