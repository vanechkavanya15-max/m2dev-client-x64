#pragma once

#include <cstdint>
#include <span>
#include <functional>

namespace Network::Handlers {

#pragma pack(push, 1)

/**
 * @brief Represents a single element in the sync position packet.
 */
struct SyncPositionElement {
    uint32_t targetId; /**< The unique identifier of the target actor. */
    int32_t x;         /**< The x-coordinate to sync to. */
    int32_t y;         /**< The y-coordinate to sync to. */
};

/**
 * @brief The header of the sync position packet.
 */
struct SyncPositionPacket {
    uint16_t header; /**< Packet header identifier. */
    uint16_t length; /**< Total length of the packet including header and elements. */
};

#pragma pack(pop)

/**
 * @brief Handler for position synchronization network packets.
 * 
 * Responsible for parsing the sync position packet and notifying the system
 * about position adjustments to resolve desynchronization with the server.
 * This class follows the Single Responsibility Principle and is decoupled from the GUI.
 */
class SyncPositionHandler {
public:
    /**
     * @brief Callback type for when a position synchronization is received.
     * @param targetId The unique identifier of the target actor.
     * @param x The new x-coordinate.
     * @param y The new y-coordinate.
     */
    using OnSyncPositionCallback = std::function<void(uint32_t targetId, int32_t x, int32_t y)>;

    /**
     * @brief Constructs a new SyncPositionHandler.
     * @param callback The callback to invoke when a position is synced.
     */
    explicit SyncPositionHandler(OnSyncPositionCallback callback);

    /**
     * @brief Handles the incoming sync position network packet.
     * @param payload The binary data of the packet payload, starting with the packet header.
     * @return true if the packet was successfully parsed and handled, false otherwise.
     */
    bool HandlePacket(std::span<const uint8_t> payload) const;

private:
    OnSyncPositionCallback onSyncPosition;
};

} // namespace Network::Handlers
