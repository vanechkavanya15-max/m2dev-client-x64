#pragma once

#include <cstdint>
#include <span>

class CPythonPlayer;

namespace Network::Packets
{
#pragma pack(push, 1)

    /// @brief Player skill structure for network communication
    struct PlayerSkill
    {
        uint8_t masterType;
        uint8_t level;
        uint32_t nextReadTime;
    };

    /// @brief Packet containing old skill level structure
    struct SkillLevelPacket
    {
        uint16_t header;
        uint16_t length;
        uint8_t skillLevels[255];
    };

    /// @brief Packet containing new skill level structure
    struct SkillLevelNewPacket
    {
        uint16_t header;
        uint16_t length;
        PlayerSkill skills[255];
    };

#pragma pack(pop)
}

namespace Network::Handlers
{
    /// @brief Represents the UI refresh notification state resulting from packet processing
    struct SkillUpdateResult
    {
        bool success;
        bool refreshSkillWindow;
        bool refreshStatus;
    };

    /// @brief Network handler for processing skill level updates from the server
    class SkillLevelHandler
    {
    public:
        /// @brief Processes the old format skill level packet
        /// @param player Reference to the active player instance
        /// @param buffer Network buffer containing the packet data
        /// @return A result structure indicating parsing success and UI refresh requirements
        static SkillUpdateResult HandleSkillLevel(CPythonPlayer& player, std::span<const uint8_t> buffer);

        /// @brief Processes the new format skill level packet
        /// @param player Reference to the active player instance
        /// @param buffer Network buffer containing the packet data
        /// @return A result structure indicating parsing success and UI refresh requirements
        static SkillUpdateResult HandleSkillLevelNew(CPythonPlayer& player, std::span<const uint8_t> buffer);
    };
}
