#pragma once

#include <cstdint>
#include <tuple>
#include <span>

namespace Network
{
    namespace Handlers
    {
        /**
         * @brief Structure representing the handshake synchronization packet.
         * 
         * This packet is used to calculate and synchronize the time difference
         * between the local client and the server.
         */
        #pragma pack(push, 1)
        struct HandshakePacket
        {
            uint8_t  header;       /**< @brief Packet header identifier. */
            uint32_t handshakeId;  /**< @brief Unique identifier for the handshake session. */
            uint32_t serverTime;   /**< @brief The current time provided by the server (in ms). */
            int32_t  delta;        /**< @brief The time difference/delay used for compensation. */
        };
        #pragma pack(pop)

        /**
         * @brief Structure representing the internal time synchronization state.
         */
        struct TimeSyncState
        {
            uint32_t baseServerTime; /**< @brief The base server time locked at handshake. */
            uint32_t localClientTime; /**< @brief The local client time recorded at handshake. */
            bool     isSynchronized;  /**< @brief Indicates whether time has been synced. */
        };

        /**
         * @brief Calculates the local time delta based on the handshake packet.
         * 
         * Computes the new synchronized server time by taking the provided server time
         * and the given delta (e.g. latency compensation). This explicitly avoids updating
         * any Python or UI states, strictly handling the data transformation.
         * 
         * @param packet The incoming handshake packet containing time data.
         * @param currentLocalTime The current millisecond tick count of the local machine.
         * @return A tuple containing the updated handshake packet to be sent back and the new synchronization state.
         */
        [[nodiscard]] std::tuple<HandshakePacket, TimeSyncState> HandleHandshakeTimeSync(const HandshakePacket& packet, uint32_t currentLocalTime) noexcept;
    }
}
