#pragma once

#include <cstdint>
#include <span>

/**
 * @brief Header constant for Ping (from Server to Client).
 */
constexpr uint16_t HEADER_GC_PING = 0x0007;

/**
 * @brief Header constant for Pong (from Client to Server).
 */
constexpr uint16_t HEADER_CG_PONG = 0x0006;

#pragma pack(push, 1)
/**
 * @brief Network packet structure for a Ping request from the server.
 */
struct PingPacket
{
    uint16_t header;
    uint16_t length;
    uint32_t serverTime;
};

/**
 * @brief Network packet structure for a Pong response to the server.
 */
struct PongPacket
{
    uint16_t header;
    uint16_t length;
};
#pragma pack(pop)

/**
 * @brief Interface for network stream operations.
 *
 * This interface abstracts the low-level network send and receive operations,
 * allowing handlers to be completely decoupled from specific network implementations
 * such as CNetworkStream or CPythonNetworkStream.
 */
class INetworkStream
{
public:
    virtual ~INetworkStream() = default;

    /**
     * @brief Sends data over the network.
     *
     * @param buffer A span of constant bytes representing the data to send.
     * @return true if the data was successfully queued for sending, false otherwise.
     */
    virtual bool Send(std::span<const uint8_t> buffer) = 0;

    /**
     * @brief Receives data from the network.
     *
     * @param buffer A span of bytes where the received data will be stored.
     * @return true if the expected amount of data was successfully received, false otherwise.
     */
    virtual bool Recv(std::span<uint8_t> buffer) = 0;
};

/**
 * @brief Handler for network ping packets from the server.
 *
 * This class is responsible for processing incoming GC::PING packets. It extracts
 * the server time, synchronizes the local timer, and responds with a CG::PONG packet
 * to measure round-trip time and keep the connection alive.
 * It adheres to the Single Responsibility Principle and is completely decoupled
 * from UI components.
 */
class PingHandler
{
public:
    /**
     * @brief Handles an incoming ping request and sends a pong response.
     *
     * @param stream The network stream interface used for reading the ping packet and writing the pong packet.
     * @param lastPingTime Reference to an integer that will be updated with the time (in milliseconds) when the ping was received.
     * @return true if the ping was successfully handled and a pong was sent, false if a network error occurred.
     */
    bool HandlePing(INetworkStream& stream, uint32_t& lastPingTime);
};
