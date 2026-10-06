#include "StdAfx.h"
#include "HandshakeHandler.h"

namespace Network
{
    namespace Handlers
    {
        /**
         * @brief Calculates the local time delta based on the handshake packet.
         * 
         * @param packet The incoming handshake packet containing time data.
         * @param currentLocalTime The current millisecond tick count of the local machine.
         * @return A tuple containing the updated handshake packet to be sent back and the new synchronization state.
         */
        [[nodiscard]] std::tuple<HandshakePacket, TimeSyncState> HandleHandshakeTimeSync(const HandshakePacket& packet, uint32_t currentLocalTime) noexcept
        {
            // Calculate the compensated time based on the server's time and the round-trip or offset delta.
            // In the original game protocol, the delta is added to the server time, and then we add it again
            // to account for the one-way trip delay (approximated as delta).
            uint32_t compensatedServerTime = packet.serverTime + packet.delta;

            // Prepare the state to update memory without touching the UI directly.
            TimeSyncState newState{
                .baseServerTime = compensatedServerTime,
                .localClientTime = currentLocalTime,
                .isSynchronized = true
            };

            // Prepare the packet to be sent back to the server (Time Sync / Pong equivalent).
            // Usually, the time in the packet is updated to the newly computed time.
            HandshakePacket responsePacket = packet;
            responsePacket.serverTime = compensatedServerTime;

            return {responsePacket, newState};
        }
    }
}
