#include "StdAfx.h"
/**
 * @file GuildWarHandler.cpp
 * @brief Handler for Guild War network packets.
 *
 * This file contains the implementation of the GuildWarHandler class which
 * is responsible for processing incoming guild war state changes from the server.
 */

#include "../../Packet.h"
#include "../../PythonGuild.h"
#include <cstdint>
#include <span>
#include <string_view>

namespace Network::Handlers
{
    /**
     * @class GuildWarHandler
     * @brief Processes incoming TPacketGCGuildWar network packets.
     *
     * Decouples the network logic from GUI layers by updating only the C++ 
     * memory state in CPythonGuild. Python/GUI updates should be handled 
     * asynchronously via event listeners or pointer notifications.
     */
    class GuildWarHandler
    {
    public:
        /**
         * @brief Handles the guild war packet and updates the guild war state.
         *
         * @param packet The incoming guild war packet from the server.
         * @return true if the packet was successfully parsed and handled.
         * @return false if an error occurred during processing.
         */
        static bool HandlePacket(const TPacketGCGuildWar& packet)
        {
            // Zero-conflict & Zero Hungarian notation rule applied
            const uint32_t guildSelf = packet.dwGuildSelf;
            const uint32_t guildOpp = packet.dwGuildOpp;
            const uint8_t type = packet.bType;
            const uint8_t warState = packet.bWarState;

            switch (warState)
            {
                case GUILD_WAR_SEND_DECLARE:
                    // Currently no C++ state changes; GUI was previously called.
                    break;

                case GUILD_WAR_RECV_DECLARE:
                    // Currently no C++ state changes; GUI was previously called.
                    break;

                case GUILD_WAR_ON_WAR:
                    CPythonGuild::Instance().StartGuildWar(guildOpp);
                    break;

                case GUILD_WAR_END:
                    CPythonGuild::Instance().EndGuildWar(guildOpp);
                    break;

                default:
                    // Handled gracefully for other states (GUILD_WAR_NONE, REFUSE, WAIT_START, CANCEL)
                    break;
            }

            return true;
        }
    };
}
