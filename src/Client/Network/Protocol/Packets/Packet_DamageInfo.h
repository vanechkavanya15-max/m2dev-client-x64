#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Packet sent from server to client to broadcast damage information.
 * 
 * This packet provides details about the damage inflicted on a target,
 * including the target's unique identifier, the amount of damage, and
 * flags (e.g., critical hit).
 */
typedef struct packet_damage_info
{
    union {
        uint16_t header;
        uint16_t byHeader; // Legacy support
    };
    uint16_t length;
    union {
        uint32_t targetId;
        uint32_t dwVID; // Legacy support
    };
    uint8_t flag;
    int32_t damage;
} TPacketGCDamageInfo;

#pragma pack(pop)
