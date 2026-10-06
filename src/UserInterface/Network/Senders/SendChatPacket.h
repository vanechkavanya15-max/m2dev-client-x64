#pragma once

#include "../../../EterBase/Result.h"
#include "../../Packets/Packet_Chat.h"
#include <string_view>
#include <functional>
#include <span>

namespace Network::Senders
{
    /**
     * @brief Formats and sends a chat packet to the server.
     * 
     * @param text The chat message content.
     * @param type The type of the chat message (e.g., Talking, Shout, Guild).
     * @param sendCallback A callback function to dispatch the binary payload over the network stream.
     * @return EterBase::PacketResult<void> Returns void on success, or an appropriate PacketError on failure.
     */
    EterBase::PacketResult<void> SendChatPacket(
        std::string_view text, 
        Network::Packets::ChatType type, 
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
