#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <cstring>

namespace Movement {

/**
 * @brief Represents a 3D position in the game world.
 */
struct Position {
    float x;
    float y;
    float z;
};

/**
 * @brief Represents a 3D velocity vector.
 */
struct Velocity {
    float x;
    float y;
    float z;
};

#pragma pack(push, 1)
/**
 * @brief Network packet structure for movement synchronization.
 * Strictly packed to 1 byte alignment for network transmission.
 */
struct PacketGCMovementSync {
    uint8_t header;         ///< Packet header identifier
    uint32_t targetId;      ///< Identifier of the entity being moved
    uint32_t serverTime;    ///< Server time at the moment of synchronization
    float startX;           ///< Starting X position
    float startY;           ///< Starting Y position
    float startZ;           ///< Starting Z position
    float velocityX;        ///< Velocity along X axis
    float velocityY;        ///< Velocity along Y axis
    float velocityZ;        ///< Velocity along Z axis
};
#pragma pack(pop)

/**
 * @brief Holds the current state required for dead reckoning calculation.
 */
struct DeadReckoningState {
    Position startPosition;
    Velocity velocity;
    uint32_t lastSyncTime;
};

/**
 * @brief Class responsible for extrapolating entity movement (Dead Reckoning).
 * Decoupled from GUI and Python, operating purely on C++ memory state.
 */
class DeadReckoning {
public:
    /**
     * @brief Parses a binary buffer into a movement synchronization packet.
     * @param buffer A span over the raw byte buffer received from network.
     * @return An optional containing the packet if parsed successfully, std::nullopt otherwise.
     */
    static std::optional<PacketGCMovementSync> ParseSyncPacket(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(PacketGCMovementSync)) {
            return std::nullopt;
        }
        
        PacketGCMovementSync packet{};
        std::memcpy(&packet, buffer.data(), sizeof(PacketGCMovementSync));
        return packet;
    }

    /**
     * @brief Updates the dead reckoning state based on a received sync packet.
     * @param state The state object to update.
     * @param packet The validated synchronization packet.
     */
    static void UpdateState(DeadReckoningState& state, const PacketGCMovementSync& packet) {
        state.startPosition = { packet.startX, packet.startY, packet.startZ };
        state.velocity = { packet.velocityX, packet.velocityY, packet.velocityZ };
        state.lastSyncTime = packet.serverTime;
    }

    /**
     * @brief Extrapolates the current position based on elapsed time.
     * @param state The current dead reckoning state.
     * @param currentTime The current client/server synchronized time in milliseconds.
     * @return The predicted position.
     */
    static Position Extrapolate(const DeadReckoningState& state, uint32_t currentTime) {
        if (currentTime <= state.lastSyncTime) {
            return state.startPosition;
        }

        // Calculate delta time in seconds
        float deltaTimeSeconds = static_cast<float>(currentTime - state.lastSyncTime) / 1000.0f;

        Position predictedPosition;
        predictedPosition.x = state.startPosition.x + state.velocity.x * deltaTimeSeconds;
        predictedPosition.y = state.startPosition.y + state.velocity.y * deltaTimeSeconds;
        predictedPosition.z = state.startPosition.z + state.velocity.z * deltaTimeSeconds;

        return predictedPosition;
    }
};

} // namespace Movement
