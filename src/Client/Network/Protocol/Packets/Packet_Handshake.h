#pragma once

#include <cstdint>

/**
 * @file Packet_Handshake.h
 * @brief Modernized C++20 network handshake and time synchronization structures.
 * 
 * Complies with single responsibility principle and strict struct alignment rules.
 * Uses <cstdint> strictly without Win32 Hungarian notation.
 */

#pragma pack(push, 1)

/**
 * @brief Represents the data exchanged during the client-server time sync handshake.
 * 
 * Ensures strict memory alignment without padding. Based strictly on the legacy 
 * TPacketGCHandshake structure (13 bytes total: 1 byte header, 12 bytes payload).
 */
struct PacketHandshake
{
    /** @brief Protocol operation header identifier (1 byte to match legacy protocol). */
    uint8_t header;
    
    /** @brief Unique handshake iteration or sequence identifier. */
    uint32_t handshake;
    
    /** @brief Current server time in milliseconds. */
    uint32_t time;
    
    /** @brief Time delta / offset calculated to adjust client latency. */
    int32_t delta;
};

/**
 * @brief Stores smooth time synchronization states for the network stream.
 */
struct ServerTimeSync
{
    /** @brief The server time captured at the moment of the handshake. */
    uint32_t changeServerTime;
    
    /** @brief The client local time captured at the moment of the handshake. */
    uint32_t changeClientTime;
};

#pragma pack(pop)
