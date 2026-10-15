#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Represents a single element in a position synchronization packet.
 *
 * This structure holds the target ID and its updated X and Y coordinates.
 * It is used for forcing position sync to avoid rubberbanding.
 */
struct SyncPositionElement
{
    /** @brief The unique identifier of the target (e.g., player or entity). */
    uint32_t id;

    /** @brief The X coordinate of the position. */
    int32_t x;

    /** @brief The Y coordinate of the position. */
    int32_t y;
};

/**
 * @brief Client-to-Server (CG) packet for synchronizing positions.
 *
 * Sent by the client timer to the server to prevent rubberbanding issues
 * during high ping or desync scenarios.
 */
struct CGSyncPositionPacket
{
    /** @brief The packet header identifier. */
    uint16_t header;

    /** @brief The total size of the packet. */
    uint16_t length;
};

/**
 * @brief Server-to-Client (GC) packet for synchronizing positions.
 *
 * Received from the server to enforce a position update on the client side.
 */
struct GCSyncPositionPacket
{
    /** @brief The packet header identifier. */
    uint16_t header;

    /** @brief The total size of the packet. */
    uint16_t length;
};

#pragma pack(pop)
