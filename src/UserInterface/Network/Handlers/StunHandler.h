#pragma once
#include <cstdint>
#include <string_view>
#include <span>

// Forward declarations to decouple from heavy includes
class CPythonNetworkStream;
class CInstanceBase;

/**
 * @brief Handler for processing the Stun network packet from the server.
 * 
 * Extracts the virtual ID of the stunned actor and updates the game character manager
 * state accordingly without tightly coupling to the Python UI system.
 */
class StunHandler
{
public:
    /**
     * @brief Processes the stun packet data.
     * @param networkStream Pointer to the network stream receiving the packet.
     * @return true if the packet was processed successfully, false otherwise.
     */
    static bool Handle(CPythonNetworkStream* networkStream);
};
