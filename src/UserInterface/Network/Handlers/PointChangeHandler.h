#pragma once

#include <cstdint>

#include "../../Packet.h"

/**
 * @brief Class responsible for handling PointChange packets.
 * Decouples Python UI calls from the network layer. Updates internal C++ state only.
 */
class PointChangeHandler {
public:
    /**
     * @brief Processes a PointChange packet received from the server.
     * Updates the local player character status and other relevant state.
     * @param pointChange The packet containing point type, amount, and value.
     * @return true if handled successfully, false otherwise.
     */
    static bool HandlePointChange(const TPacketGCPointChange& pointChange);
};
