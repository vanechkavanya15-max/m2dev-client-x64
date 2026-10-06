#include "StdAfx.h"
/**
 * @file DungeonDestinationHandler.cpp
 * @brief Handler for dungeon destination packet.
 */

#include <cstdint>
#ifndef TEST_MOCK
#include "../../PythonPlayer.h"
#endif

namespace Network {
namespace Handlers {

/**
 * @class DungeonDestinationHandler
 * @brief Handles setting the dungeon destination.
 */
class DungeonDestinationHandler {
public:
    /**
     * @brief Sets the dungeon destination position in the player instance.
     * @param x The X coordinate of the destination.
     * @param y The Y coordinate of the destination.
     * @return True if the handling was successful, false otherwise.
     */
    bool HandleDestinationPosition(uint32_t x, uint32_t y) {
        CPythonPlayer::Instance().SetDungeonDestinationPosition(static_cast<int>(x), static_cast<int>(y));
        return true;
    }
};

} // namespace Handlers
} // namespace Network
