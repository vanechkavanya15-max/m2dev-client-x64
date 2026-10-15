#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

#pragma pack(push, 1)

/**
 * @brief Represents the Ping packet sent from the server to check connection stability.
 */
struct ServerPingPacket
{
    uint16_t header;      //!< The packet header identifier.
    uint16_t length;      //!< The length of the packet.
    uint32_t serverTime;  //!< The current server time for synchronization.
};

/**
 * @brief Represents the Pong packet sent by the client in response to a Ping.
 */
struct ClientPongPacket
{
    uint16_t header;      //!< The packet header identifier.
    uint16_t length;      //!< The length of the packet.
};

#pragma pack(pop)

/**
 * @brief State structure storing the current keep-alive and ping status.
 */
struct KeepAliveState
{
    uint32_t lastPingTime = 0;      //!< Local client time when the last ping was received.
    uint32_t lastServerTime = 0;    //!< Server time received in the last ping.
    bool isConnected = false;       //!< True if keep-alive is active.
};

/**
 * @brief Handler for keep-alive operations (Ping/Pong) without GUI coupling.
 * 
 * This class handles incoming ping packets, updates internal connection state,
 * and triggers notifications through C++ callbacks rather than Python API calls.
 */
class KeepAliveHandler
{
public:
    /**
     * @brief Type definition for the ping received callback.
     * @param state The updated keep-alive state.
     */
    using OnPingReceivedCallback = std::function<void(const KeepAliveState& state)>;

    /**
     * @brief Type definition for the error callback.
     * @param errorMessage A string view detailing the error encountered.
     */
    using OnErrorCallback = std::function<void(std::string_view errorMessage)>;

    /**
     * @brief Default constructor for KeepAliveHandler.
     */
    KeepAliveHandler() = default;

    /**
     * @brief Default destructor for KeepAliveHandler.
     */
    ~KeepAliveHandler() = default;

    /**
     * @brief Sets the callback to be invoked when a ping is successfully processed.
     * @param callback The function to call on ping reception.
     */
    void setPingCallback(OnPingReceivedCallback callback)
    {
        onPingReceived = std::move(callback);
    }

    /**
     * @brief Sets the callback to be invoked when an error occurs.
     * @param callback The function to call on error.
     */
    void setErrorCallback(OnErrorCallback callback)
    {
        onError = std::move(callback);
    }

    /**
     * @brief Processes an incoming binary payload representing a Ping packet.
     * @param payload A read-only span containing the packet data.
     * @param currentClientTime The current client time in milliseconds.
     * @return True if the packet was successfully processed, false otherwise.
     */
    bool processPing(std::span<const uint8_t> payload, uint32_t currentClientTime)
    {
        if (payload.size() < sizeof(ServerPingPacket))
        {
            if (onError)
            {
                onError("Invalid ping packet size received.");
            }
            return false;
        }

        const auto* packet = reinterpret_cast<const ServerPingPacket*>(payload.data());
        
        state.lastServerTime = packet->serverTime;
        state.lastPingTime = currentClientTime;
        state.isConnected = true;

        if (onPingReceived)
        {
            onPingReceived(state);
        }

        return true;
    }

    /**
     * @brief Generates a Pong packet to reply to the server.
     * @param pongHeader The header ID for the pong packet.
     * @return A constructed ClientPongPacket ready to be sent.
     */
    ClientPongPacket createPongResponse(uint16_t pongHeader) const
    {
        ClientPongPacket response{};
        response.header = pongHeader;
        response.length = static_cast<uint16_t>(sizeof(ClientPongPacket));
        return response;
    }

    /**
     * @brief Gets the current keep-alive state.
     * @return The state structure.
     */
    [[nodiscard]] const KeepAliveState& getState() const
    {
        return state;
    }

    /**
     * @brief Resets the keep-alive state, marking the connection as disconnected.
     */
    void reset()
    {
        state = KeepAliveState{};
    }

private:
    KeepAliveState state{};                      //!< Internal state of the keep-alive connection.
    OnPingReceivedCallback onPingReceived;       //!< Callback invoked when a ping is processed.
    OnErrorCallback onError;                     //!< Callback invoked when an error occurs.
};
