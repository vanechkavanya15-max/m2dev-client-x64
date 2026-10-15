#include "ChatHandler.h"

namespace Client::Network::Handlers
{

bool IsValidUtf8(std::string_view text)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(text.data());
    std::size_t length = text.length();

    for (std::size_t i = 0; i < length;)
    {
        uint8_t c = bytes[i];
        int bytes_to_check = 0;

        if (c <= 0x7F) {
            bytes_to_check = 0;
        } else if ((c & 0xE0) == 0xC0) {
            if (c < 0xC2) return false; // Overlong encoding
            bytes_to_check = 1;
        } else if ((c & 0xF0) == 0xE0) {
            if (c == 0xE0 && (i + 1 < length && (bytes[i + 1] < 0xA0 || bytes[i + 1] > 0xBF))) return false;
            if (c == 0xED && (i + 1 < length && (bytes[i + 1] < 0x80 || bytes[i + 1] > 0x9F))) return false;
            bytes_to_check = 2;
        } else if ((c & 0xF8) == 0xF0) {
            if (c > 0xF4) return false;
            if (c == 0xF0 && (i + 1 < length && (bytes[i + 1] < 0x90 || bytes[i + 1] > 0xBF))) return false;
            if (c == 0xF4 && (i + 1 < length && (bytes[i + 1] < 0x80 || bytes[i + 1] > 0x8F))) return false;
            bytes_to_check = 3;
        } else {
            return false; // Invalid first byte
        }

        if (i + bytes_to_check >= length) {
            return false; // Incomplete sequence
        }

        for (int j = 1; j <= bytes_to_check; ++j) {
            if ((bytes[i + j] & 0xC0) != 0x80) {
                return false; // Invalid continuation byte
            }
        }

        i += bytes_to_check + 1;
    }

    return true;
}

EterBase::PacketResult<void> ProcessChatMessage(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(PacketChatHeader))
    {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* header = reinterpret_cast<const PacketChatHeader*>(buffer.data());

    if (header->length < sizeof(PacketChatHeader))
    {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    if (buffer.size() < header->length)
    {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    std::size_t messageLen = header->length - sizeof(PacketChatHeader);
    if (messageLen == 0)
    {
        return {};
    }

    std::string_view messageView(reinterpret_cast<const char*>(buffer.data() + sizeof(PacketChatHeader)), messageLen);
    
    // Remove null terminator if it exists at the end of the view
    if (!messageView.empty() && messageView.back() == '\0') {
        messageView.remove_suffix(1);
    }

    if (!IsValidUtf8(messageView))
    {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    Client::Core::EventBus::GetInstance().Publish(
        ChatMessageReceivedEvent{header->type, header->dwVID, header->bEmpire, std::string(messageView)}
    );

    return {};
}

EterBase::PacketResult<void> ProcessWhisperMessage(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(PacketWhisperHeader))
    {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* header = reinterpret_cast<const PacketWhisperHeader*>(buffer.data());

    if (header->length < sizeof(PacketWhisperHeader))
    {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    if (buffer.size() < header->length)
    {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    // Safely parse name without assuming null-termination, bounded by max length
    std::size_t nameLen = 0;
    while (nameLen < sizeof(header->szNameFrom) && header->szNameFrom[nameLen] != '\0')
    {
        nameLen++;
    }
    std::string_view senderNameView(header->szNameFrom, nameLen);

    std::size_t messageLen = header->length - sizeof(PacketWhisperHeader);
    if (messageLen == 0)
    {
        return {};
    }

    std::string_view messageView(reinterpret_cast<const char*>(buffer.data() + sizeof(PacketWhisperHeader)), messageLen);

    // Remove null terminator if it exists at the end of the view
    if (!messageView.empty() && messageView.back() == '\0') {
        messageView.remove_suffix(1);
    }

    if (!IsValidUtf8(messageView))
    {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    Client::Core::EventBus::GetInstance().Publish(
        WhisperMessageReceivedEvent{header->type, std::string(senderNameView), std::string(messageView)}
    );

    return {};
}

} // namespace Client::Network::Handlers
