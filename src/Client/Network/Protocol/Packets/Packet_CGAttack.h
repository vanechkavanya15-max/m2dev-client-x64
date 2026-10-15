#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Represents a client-to-server attack packet.
 * 
 * This packet is sent by the client when attacking a target.
 * It contains the attack type, target ID, and CRC checks for anti-cheat verification.
 */
typedef struct command_attack
{
    union {
        uint16_t header;
        uint16_t bHeader; // Legacy support
    };
    union {
        uint16_t length;
        uint16_t wSize; // Legacy support
    };
    union {
        uint8_t type;
        uint8_t bType; // Legacy support
    };
    union {
        uint32_t targetId;
        uint32_t dwVictimVID; // Legacy support
    };
    union {
        uint8_t crcMagicCubeProcPiece;
        uint8_t bCRCMagicCubeProcPiece; // Legacy support
    };
    union {
        uint8_t crcMagicCubeFilePiece;
        uint8_t bCRCMagicCubeFilePiece; // Legacy support
    };
} TPacketCGAttack;

#pragma pack(pop)
