#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "ChatHandler.h"

namespace Client::Network::Handlers
{

class ChatMessagePacketHandler
{
public:
    ChatMessagePacketHandler() = default;
    ~ChatMessagePacketHandler() = default;

    // Usun konstruktory kopiujace i przenoszace
    ChatMessagePacketHandler(const ChatMessagePacketHandler&) = delete;
    ChatMessagePacketHandler& operator=(const ChatMessagePacketHandler&) = delete;
    ChatMessagePacketHandler(ChatMessagePacketHandler&&) = delete;
    ChatMessagePacketHandler& operator=(ChatMessagePacketHandler&&) = delete;

    [[nodiscard]] static EterBase::PacketResult<void> HandleNormal(std::span<const uint8_t> buffer);
    [[nodiscard]] static EterBase::PacketResult<void> HandleWhisper(std::span<const uint8_t> buffer);
    [[nodiscard]] static EterBase::PacketResult<void> HandleShout(std::span<const uint8_t> buffer);
};

} // namespace Client::Network::Handlers
