#pragma once

#include <cstdint>

/**
 * @file Packet_TargetHP.h
 * @brief Network packet structures for updating target Health Points (HP).
 *
 * This file contains modernized C++20 definitions for network packets
 * responsible for transmitting the health status of a targeted entity.
 * It complies with the strict 1-byte alignment required by the protocol.
 */

namespace Combat::Packets
{

#pragma pack(push, 1)

/**
 * @struct TargetHpPacket
 * @brief Standard packet sent by the server to update the HP percentage of a target.
 */
struct TargetHpPacket
{
    /// @brief Packet header identifier.
    uint16_t header;

    /// @brief Total length of the packet.
    uint16_t length;

    /// @brief Unique identifier of the target entity.
    uint32_t targetId;

    /// @brief Current health of the target expressed as a percentage (0-100).
    uint8_t hpPercent;
};

/**
 * @struct TargetHpBeaviumPacket
 * @brief Extended packet (Beavium protocol) for precise target HP updates.
 */
struct TargetHpBeaviumPacket
{
    /// @brief Packet header identifier.
    uint8_t header;

    /// @brief Unique identifier of the target entity.
    uint32_t targetId;

    /// @brief Exact current health points of the target.
    int64_t currentHp;
};

#pragma pack(pop)

} // namespace Combat::Packets
