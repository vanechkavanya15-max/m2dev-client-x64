#pragma once

#include "../../StdAfx.h"
#include "../../BeaviumProtocol.h"
#include <cstdint>
#include <functional>
#include <span>

namespace Network
{
    /**
     * @brief Formats and sends a movement packet to the server using the Beavium protocol.
     * 
     * @param targetPosition The pixel position of the movement target in local space.
     * @param rotation The facing rotation angle in degrees.
     * @param func The function/state of movement (e.g. FUNC_MOVE, FUNC_WAIT).
     * @param arg Additional argument for the state.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return true if the packet was successfully sent, false otherwise.
     */
    bool SendMovePacket(const TPixelPosition& targetPosition, float rotation, uint8_t func, uint16_t arg, const std::function<bool(std::span<const uint8_t>)>& sendCallback);
}
