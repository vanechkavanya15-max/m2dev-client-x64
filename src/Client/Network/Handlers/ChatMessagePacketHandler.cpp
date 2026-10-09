#include "ChatMessagePacketHandler.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers
{

EterBase::PacketResult<void> ChatMessagePacketHandler::HandleNormal(std::span<const uint8_t> buffer)
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
    
    // Usun znak null jesli istnieje na koncu widoku
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

EterBase::PacketResult<void> ChatMessagePacketHandler::HandleWhisper(std::span<const uint8_t> buffer)
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

    // Bezpieczne parsowanie nazwy bez zakladania null-terminacji, ograniczone przez max dlugosc
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

    // Usun znak null jesli istnieje na koncu widoku
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

EterBase::PacketResult<void> ChatMessagePacketHandler::HandleShout(std::span<const uint8_t> buffer)
{
    // Krzyki dziela te sama logike i strukture naglowka co HandleNormal.
    return HandleNormal(buffer);
}

} // namespace Client::Network::Handlers
