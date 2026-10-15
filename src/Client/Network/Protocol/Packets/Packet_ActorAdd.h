#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * @brief Packet sent from Server to Client when a character (actor) is added to the scene.
 *
 * This struct represents `HEADER_GC_CHARACTER_ADD`. It informs the client about
 * a new entity appearing in the visual range, including its position, type,
 * race, and speed parameters.
 */
struct TPacketGCCharacterAdd
{
    /** @brief Packet header identifier. */
    uint16_t header;
    /** @brief Length of the packet. */
    uint16_t length;
    
    /** @brief Virtual ID (VID) of the character. */
    uint32_t id;

    /** @brief Rotation angle of the character. */
    float angle;
    /** @brief X-coordinate position. */
    int32_t x;
    /** @brief Y-coordinate position. */
    int32_t y;
    /** @brief Z-coordinate position. */
    int32_t z;

    /** @brief Type of the character (e.g., PC, NPC, Monster). */
    uint8_t type;
    /** @brief Race number or model ID of the character. */
    uint16_t raceNum;
    /** @brief Movement speed of the character. */
    uint8_t movingSpeed;
    /** @brief Attack speed of the character. */
    uint8_t attackSpeed;

    /** @brief Bitmask indicating the current state flags of the character. */
    uint8_t stateFlag;
    /** @brief Array of affect flags applied to the character (e.g., poison, stun). */
    uint32_t affectFlag[2];
};

#pragma pack(pop)
