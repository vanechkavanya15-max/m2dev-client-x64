#pragma once

#include <cstdint>
#include <span>

namespace Network
{
namespace Handlers
{

#pragma pack(push, 1)
/**
 * @brief Represents the incoming teleportation (warp) packet from the server.
 * 
 * Strict 1-byte alignment is enforced for direct memory mapping from network buffers.
 */
struct WarpPacket
{
    uint16_t header;    ///< Packet header identifier
    uint16_t length;    ///< Total size of the packet
    int32_t x;          ///< Target X global coordinate
    int32_t y;          ///< Target Y global coordinate
    int32_t ipAddress;  ///< Target server IP address (IPv4 encoded as 32-bit int)
    uint16_t port;      ///< Target server port
};
#pragma pack(pop)

/**
 * @brief Holds the safely parsed state of a teleportation request.
 */
struct WarpState
{
    int32_t targetX;       ///< Target global X coordinate
    int32_t targetY;       ///< Target global Y coordinate
    int32_t serverAddress; ///< Encoded IP address of the destination server
    uint16_t serverPort;   ///< Port of the destination server
    bool isValid;          ///< Indicates if the state was successfully parsed
};

/**
 * @brief Observer interface for decoupling network handling from GUI/Game logic.
 */
class IWarpObserver
{
public:
    virtual ~IWarpObserver() = default;

    /**
     * @brief Called when a valid warp request has been parsed.
     * @param state The newly parsed teleportation state.
     */
    virtual void OnWarpRequested(const WarpState& state) = 0;
};

/**
 * @brief Handler for processing Warp (teleportation) network packets.
 * 
 * Implements the single responsibility of parsing teleportation data
 * and updating internal state while notifying decoupled observers.
 */
class WarpHandler
{
public:
    /**
     * @brief Constructs a new WarpHandler.
     */
    WarpHandler();

    /**
     * @brief Parses the incoming byte buffer for warp data.
     * @param buffer A span over the incoming network bytes.
     * @return true if the packet was successfully parsed, false otherwise.
     */
    bool HandlePacket(std::span<const uint8_t> buffer);

    /**
     * @brief Sets the observer to notify upon successful warp parsing.
     * @param observer Pointer to the observer instance.
     */
    void SetObserver(IWarpObserver* observer);

    /**
     * @brief Retrieves the current warp state.
     * @return The most recent warp state parsed.
     */
    const WarpState& GetState() const;

private:
    WarpState state_;            ///< Internal memory state of the warp request
    IWarpObserver* observer_;    ///< Decoupled observer for notifications
};

} // namespace Handlers
} // namespace Network
