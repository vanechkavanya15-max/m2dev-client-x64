#include "../StdAfx.h"
#include "SocialCommandEncoder.h"
#include <cstring>

namespace Client::Network
{
    EterBase::PacketResult<std::vector<uint8_t>> SocialCommandEncoder::EncodeChat(const ChatCommand& command)
    {
        size_t messageLen = command.message.length();
        size_t payloadSize = sizeof(TPacketCGChat) + messageLen;
        
        if (payloadSize > 0xFFFF)
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketCGChat header;
        header.header = CG::CHAT;
        header.length = static_cast<uint16_t>(payloadSize);
        header.type = command.type;

        std::vector<uint8_t> buffer(payloadSize);
        std::memcpy(buffer.data(), &header, sizeof(TPacketCGChat));
        if (messageLen > 0)
        {
            std::memcpy(buffer.data() + sizeof(TPacketCGChat), command.message.data(), messageLen);
        }

        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> SocialCommandEncoder::EncodeWhisper(const WhisperCommand& command)
    {
        size_t messageLen = command.message.length();
        size_t payloadSize = sizeof(TPacketCGWhisper) + messageLen;

        if (payloadSize > 0xFFFF)
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketCGWhisper header;
        header.header = CG::WHISPER;
        header.length = static_cast<uint16_t>(payloadSize);
        
        std::memset(header.szNameTo, 0, sizeof(header.szNameTo));
        std::strncpy(header.szNameTo, command.targetName.c_str(), CHARACTER_NAME_MAX_LEN);
        header.szNameTo[CHARACTER_NAME_MAX_LEN] = '\0';

        std::vector<uint8_t> buffer(payloadSize);
        std::memcpy(buffer.data(), &header, sizeof(TPacketCGWhisper));
        if (messageLen > 0)
        {
            std::memcpy(buffer.data() + sizeof(TPacketCGWhisper), command.message.data(), messageLen);
        }

        return buffer;
    }
}
