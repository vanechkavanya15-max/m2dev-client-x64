#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Packet for Server to Client Movement (HEADER_GC_CHARACTER_MOVE)
 * 
 * This packet provides details regarding character movement. It handles
 * the target ID, the position coordinates, the time reference, and the
 * duration.
 */
typedef struct packet_move
{
    /** @brief Packet header identifier. */
    uint16_t header;
    /** @brief Packet length. */
    uint16_t length;
    /** @brief Movement function type. */
    uint8_t func;
    /** @brief Movement argument. */
    uint8_t arg;
    /** @brief Rotation byte. */
    uint8_t rot;
    /** @brief Target virtual identifier (Entity ID). */
    uint32_t vid;
    /** @brief X-coordinate position. */
    int32_t x;
    /** @brief Y-coordinate position. */
    int32_t y;
    /** @brief Time stamp. */
    uint32_t time;
    /** @brief Duration of movement. */
    uint32_t duration;
} TPacketGCMove;

#pragma pack(pop)
