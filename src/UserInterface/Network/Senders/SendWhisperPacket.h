#pragma once

#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include <string_view>
#include <span>
#include <functional>
#include <cstdint>

namespace Network::Senders
{
    /**
     * @brief Constructs and sends a whisper (private message) packet to a specific player.
     * 
     * Applies C++23 guidelines, replaces archaic outputs with EterBase::PacketResult, 
     * avoids Hungarian notation, and utilizes std::string_view for strings.
     * 
     * @param targetName The name of the player to send the message to.
     * @param message The content of the whisper message.
     * @param sendCallback A callback function capable of transmitting a span of bytes over the network stream.
     * 
     * @return EterBase::PacketResult<void> indicating success or a specific PacketError.
     */
    EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
