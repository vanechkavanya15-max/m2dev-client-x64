#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Represents the client-to-server movement packet.
 * 
 * This structure is used to send character movement data, including position,
 * rotation, and movement type (function) to the server. It uses strict 1-byte
 * alignment to ensure correct memory layout for network transmission.
 */
struct TPacketCGMove
{
    /** @brief Packet header identifier. */
    uint16_t header;
    
    /** @brief Total length of the packet in bytes. */
    uint16_t length;
    
    /** @brief Movement function (e.g., walk, run). */
    uint8_t  func;
    
    /** @brief Additional argument for the movement function. */
    uint8_t  arg;
    
    /** @brief Rotation angle of the character. */
    uint8_t  rot;
    
    /** @brief X-coordinate in global space. Can be negative. */
    int32_t  x;
    
    /** @brief Y-coordinate in global space. Can be negative. */
    int32_t  y;
    
    /** @brief Client-side timestamp of the movement. */
    uint32_t time;
};

#pragma pack(pop)
