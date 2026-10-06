#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Represents a packet notifying the client that an entity has died.
 */
struct TPacketGCDead
{
    /** @brief Network packet header (HEADER_GC_DEAD). */
    uint16_t header;
    
    /** @brief Size of the packet including header and length fields. */
    uint16_t length;
    
    /** @brief Virtual ID of the entity that died. */
    uint32_t targetId;
};

#pragma pack(pop)
