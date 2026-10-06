#pragma once

#include <cstdint>
#include <span>
#include "../../BeaviumProtocol.h"

class CNetworkStream; // Forward declaration from EterLib/NetStream.h

/**
 * @brief Handler responsible for sending attack packets securely and correctly aligned to the server.
 * 
 * This class applies SRP (Single Responsibility Principle) by exclusively dealing with network
 * transport for attack requests. It separates the game's GUI and visual elements from network logic.
 */
class SendAttackHandler
{
public:
    /**
     * @brief Sends an attack request packet to the server for a specific victim.
     * 
     * Applies C++20 guidelines, avoids Hungarian notation, and converts structural data securely
     * into std::span arrays for network transmission.
     * 
     * @param targetId      The Virtual ID of the target victim being attacked.
     * @param attackMotion  The motion index of the attack, used for alternative packet if greater than 0.
     * @param sequence      The packet synchronization sequence/CRC counter.
     * @param networkStream Pointer to the network stream used to send the payload.
     * 
     * @return true if the packet transmission request was successfully queued, false otherwise.
     */
    static bool SendAttack(uint32_t targetId, uint32_t attackMotion, uint16_t sequence, CNetworkStream* networkStream);
};
