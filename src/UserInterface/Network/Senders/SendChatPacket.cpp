#include "../../StdAfx.h"
#include "SendChatPacket.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include <vector>
#include <cstring>

namespace Network::Senders
{
    EterBase::PacketResult<void> SendChatPacket(
        std::string_view text, 
        Network::Packets::ChatType type, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (text.empty())
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Attempted to send an empty chat message.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (type >= Network::Packets::ChatType::MaxNum)
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Invalid chat type provided.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const size_t structSize = sizeof(Network::Packets::ChatCS);
        const size_t totalLength = structSize + text.size();

        if (totalLength > UINT16_MAX)
        {
            EterBase::ModernLogger::Warn("SendChatPacket: Chat message exceeds maximum allowed packet length.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::vector<uint8_t> packetBuffer(totalLength);
        
        auto* packetHeader = reinterpret_cast<Network::Packets::ChatCS*>(packetBuffer.data());
        packetHeader->header = static_cast<uint16_t>(Network::Packets::ChatHeader::CG_CHAT);
        packetHeader->length = static_cast<uint16_t>(totalLength);
        packetHeader->type = static_cast<uint8_t>(type);

        std::memcpy(packetBuffer.data() + structSize, text.data(), text.size());

        std::span<const uint8_t> packetSpan(packetBuffer);

        if (!sendCallback(packetSpan))
        {
            EterBase::ModernLogger::Error("SendChatPacket: Network stream failed to send chat packet (type: {}).", static_cast<uint8_t>(type));
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        EterBase::ModernLogger::Debug("SendChatPacket: Successfully sent chat message of length {} (type: {}).", text.size(), static_cast<uint8_t>(type));
        return {};
    }
}
