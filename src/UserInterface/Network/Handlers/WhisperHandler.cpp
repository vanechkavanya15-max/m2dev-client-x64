#include "../../StdAfx.h"
#include "WhisperHandler.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessWhisperMessage(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketWhisperHeader))
        {
            EterBase::ModernLogger::Error("ProcessWhisperMessage: Buffer size {} is less than header size {}", buffer.size(), sizeof(PacketWhisperHeader));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketWhisperHeader*>(buffer.data());

        if (packet->length < sizeof(PacketWhisperHeader))
        {
            EterBase::ModernLogger::Error("ProcessWhisperMessage: Packet length {} is less than header size {}", packet->length, sizeof(PacketWhisperHeader));
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (buffer.size() < packet->length)
        {
            EterBase::ModernLogger::Error("ProcessWhisperMessage: Buffer size {} is less than packet length {}", buffer.size(), packet->length);
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        // Secure string parsing without assuming null-termination
        size_t nameLen = strnlen(packet->senderName, sizeof(packet->senderName));
        std::string_view senderNameView(packet->senderName, nameLen);

        size_t messageLen = packet->length - sizeof(PacketWhisperHeader);
        const char* textPtr = reinterpret_cast<const char*>(buffer.data() + sizeof(PacketWhisperHeader));
        std::string_view messageView(textPtr, messageLen);

        UserInterface::Core::EventBus::GetInstance().Publish(
            WhisperMessageReceivedEvent{packet->type, std::string(senderNameView), std::string(messageView)}
        );

        return {};
    }
}
