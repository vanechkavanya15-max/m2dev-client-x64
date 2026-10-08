#pragma once

#include "../StdAfx.h"
#include "../../UserInterface/Packet.h"
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#include "../../EterBase/Result.h"


namespace Client::Network
{
    /**
     * @brief Structure representing a chat command to be encoded.
     */
    struct ChatCommand
    {
        uint8_t type;
        std::string message;
    };

    /**
     * @brief Structure representing a whisper command to be encoded.
     */
    struct WhisperCommand
    {
        std::string targetName;
        std::string message;
    };

    /**
     * @brief Encoder for social-related commands like chat and whisper.
     * Complies with ZERO-CONFLICT and modern C++23 standards.
     */
    class SocialCommandEncoder
    {
    public:
        /**
         * @brief Encodes a CG::CHAT packet with a dynamic message payload.
         * @param command The chat command data.
         * @return A PacketResult containing the encoded byte buffer, or an error.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeChat(const ChatCommand& command);

        /**
         * @brief Encodes a CG::WHISPER packet with a dynamic message payload.
         * @param command The whisper command data.
         * @return A PacketResult containing the encoded byte buffer, or an error.
         */
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeWhisper(const WhisperCommand& command);
    };
}
