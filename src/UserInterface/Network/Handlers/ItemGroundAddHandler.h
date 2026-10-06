#pragma once

#include <cstdint>
#include <span>

/**
 * @brief Represents the data structure for dropping an item on the ground.
 * 
 * This structure strictly defines the packet layout expected from the network stream
 * when an item is added to the ground. It uses exact alignment matching the C++20 standard
 * and network protocols.
 */
#pragma pack(push, 1)
struct ItemGroundAddPacket
{
    /** @brief The protocol header identifying this specific packet. */
    uint8_t header;

    /** @brief The X coordinate in the global world. */
    int32_t x;

    /** @brief The Y coordinate in the global world. */
    int32_t y;

    /** @brief The Z coordinate in the global world. */
    int32_t z;

    /** @brief The unique virtual identifier of the item instance. */
    uint32_t id;
    /** @brief The virtual number of the item. */
    uint32_t vnum;
};
#pragma pack(pop)

/**
 * @brief Handles the 'Item Ground Add' network packet.
 * 
 * This handler processes the network packet that signifies an item has been dropped
 * onto the ground in the game world. It updates the internal 3D/C++ game state
 * without direct coupling to the Python UI.
 */
class ItemGroundAddHandler
{
public:
    /**
     * @brief Processes the item ground add packet buffer.
     * 
     * @param buffer The binary span representing the network packet payload.
     * @return true if the buffer was valid and successfully parsed; false otherwise.
     */
    static bool Handle(std::span<const uint8_t> buffer);
};
