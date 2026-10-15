#pragma once

#include <cstdint>
#include <string_view>
#include <span>

namespace Network::Packets
{
    /**
     * @brief Maximum length of a character name.
     */
    constexpr size_t CHARACTER_NAME_MAX_LEN = 64;

    /**
     * @brief Header codes for chat packets.
     */
    enum class ChatHeader : uint16_t
    {
        CG_CHAT = 0x0601,
        CG_WHISPER = 0x0602,
        GC_CHAT = 0x0603,
        GC_WHISPER = 0x0604,
    };

    /**
     * @brief Enum defining different types of chat messages.
     */
    enum class ChatType : uint8_t
    {
        Talking = 0,    ///< Normal talking
        Info = 1,       ///< Info (picked up item, got exp, etc)
        Notice = 2,     ///< Notice
        Party = 3,      ///< Party chat
        Guild = 4,      ///< Guild chat
        Command = 5,    ///< Command
        Shout = 6,      ///< Shout
        Whisper = 7,    ///< Whisper (client side only)
        BigNotice = 8,  ///< Big notice
        MaxNum = 9
    };

#pragma pack(push, 1)

    /**
     * @brief Client to Server Chat packet structure.
     */
    struct ChatCS
    {
        uint16_t header; ///< Packet header
        uint16_t length; ///< Packet length
        uint8_t type;    ///< Type of chat message
    };

    /**
     * @brief Client to Server Whisper packet structure.
     */
    struct WhisperCS
    {
        uint16_t header; ///< Packet header
        uint16_t length; ///< Packet length
        char targetName[CHARACTER_NAME_MAX_LEN + 1]; ///< Name of the whisper target
    };

    /**
     * @brief Server to Client Chat packet structure.
     */
    struct ChatSC
    {
        uint16_t header; ///< Packet header
        uint16_t length; ///< Packet length
        uint8_t type;    ///< Type of chat message
        uint32_t id;     ///< Target or Source ID (VID)
        uint8_t empire;  ///< Empire of the speaker
    };

    /**
     * @brief Server to Client Whisper packet structure.
     */
    struct WhisperSC
    {
        uint16_t header; ///< Packet header
        uint16_t length; ///< Packet length
        uint8_t type;    ///< Type of whisper
        char fromName[CHARACTER_NAME_MAX_LEN + 1]; ///< Name of the whisper sender
    };

#pragma pack(pop)

    /**
     * @brief Helper function to get text payload from a Chat packet.
     * @param packetData The entire packet data including header.
     * @param structSize Size of the base structure.
     * @return std::string_view representing the chat text.
     */
    inline std::string_view GetChatPayload(std::span<const uint8_t> packetData, size_t structSize)
    {
        if (packetData.size() <= structSize)
        {
            return {};
        }
        const char* textStr = reinterpret_cast<const char*>(packetData.data() + structSize);
        size_t len = packetData.size() - structSize;
        // Strip null terminators if present at the end
        while (len > 0 && textStr[len - 1] == '\0')
        {
            len--;
        }
        return std::string_view(textStr, len);
    }

} // namespace Network::Packets
