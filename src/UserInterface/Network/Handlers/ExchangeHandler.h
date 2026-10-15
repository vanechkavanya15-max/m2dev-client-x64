/**
 * @file ExchangeHandler.h
 * @brief Modern C++20 handler for trading/exchange packets.
 */

#pragma once

#include <cstdint>

#include "../../Packet.h"

/**
 * @class ExchangeHandler
 * @brief Handles network packets related to the exchange (trade) system.
 *
 * This class is responsible for taking raw `TPacketGCExchange` data and
 * updating the CPythonExchange memory state. It adheres to Single Responsibility
 * by separating network packet parsing from the GUI notifications.
 */
class ExchangeHandler
{
public:
    /**
     * @brief Processes an incoming exchange packet.
     * @param packet The GC exchange packet payload.
     * @return true if the packet was successfully handled, false otherwise.
     */
    static bool HandlePacket(const TPacketGCExchange& packet);

private:
    static void HandleStart(const TPacketGCExchange& packet);
    static void HandleItemAdd(const TPacketGCExchange& packet);
    static void HandleItemDel(const TPacketGCExchange& packet);
    static void HandleElkAdd(const TPacketGCExchange& packet);
    static void HandleAccept(const TPacketGCExchange& packet);
    static void HandleEnd();
    static void HandleAlready();
    static void HandleLessElk();
};
