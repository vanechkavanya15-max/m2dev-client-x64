#pragma once

#include <cstdint>
#include <span>

namespace Network
{
namespace Handlers
{

/**
 * @brief Handles the addition of items or skills to the quick slot bar.
 * 
 * This class is responsible for encapsulating the logic needed to construct
 * and send a TPacketCGQuickSlotAdd packet to the server when a user interacts
 * with their quick slots.
 */
class QuickSlotAddHandler
{
public:
    QuickSlotAddHandler() = default;
    ~QuickSlotAddHandler() = default;

    /**
     * @brief Sends a quick slot add packet to the server.
     * 
     * @param windowPosition The target position in the quick slot window.
     * @param type The type of element being added (e.g., item, skill).
     * @param position The source position of the item or skill.
     * @return true if the packet was successfully constructed and queued for sending, false otherwise.
     */
    bool SendQuickSlotAddPacket(uint8_t windowPosition, uint8_t type, uint8_t position);
};

} // namespace Handlers
} // namespace Network
