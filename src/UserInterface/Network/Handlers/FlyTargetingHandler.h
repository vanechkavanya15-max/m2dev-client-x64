#pragma once

#include <cstdint>
#include <span>
#include <vector>

#pragma pack(push, 1)

/**
 * @brief Client-to-Server packet for setting or adding a fly target.
 */
struct FlyTargetingPacketClient {
    uint16_t header;
    uint16_t length;
    uint32_t targetId;
    int32_t x;
    int32_t y;
};

/**
 * @brief Server-to-Client packet for notifying about a fly target.
 */
struct FlyTargetingPacketServer {
    uint16_t header;
    uint16_t length;
    uint32_t shooterId;
    uint32_t targetId;
    int32_t x;
    int32_t y;
};

/**
 * @brief Server-to-Client packet for creating a flying instance.
 */
struct CreateFlyPacketServer {
    uint16_t header;
    uint16_t length;
    uint8_t type;
    uint32_t startId;
    uint32_t endId;
};

#pragma pack(pop)

/**
 * @brief Handles network packet processing related to fly targeting and flying instances.
 * 
 * Follows Single Responsibility Principle by completely separating packet data parsing
 * and object state updates from the general network stream and GUI components.
 */
class FlyTargetingHandler {
public:
    /**
     * @brief Parses a FlyTargetingPacketServer and sets the fly target for the shooter.
     * @param buffer Raw byte span containing the packet data.
     * @return true if parsing and applying the target succeeded, false otherwise.
     */
    static bool HandleReceiveFlyTargeting(std::span<const uint8_t> buffer);

    /**
     * @brief Parses a FlyTargetingPacketServer and adds an additional fly target for the shooter.
     * @param buffer Raw byte span containing the packet data.
     * @return true if parsing and adding the target succeeded, false otherwise.
     */
    static bool HandleReceiveAddFlyTargeting(std::span<const uint8_t> buffer);

    /**
     * @brief Parses a CreateFlyPacketServer and creates a new fly instance between two actors.
     * @param buffer Raw byte span containing the packet data.
     * @return true if parsing and fly creation succeeded, false otherwise.
     */
    static bool HandleReceiveCreateFly(std::span<const uint8_t> buffer);

    /**
     * @brief Constructs a client-side packet to set a new fly target.
     * @param targetId The ID of the targeted character instance.
     * @param x The global X coordinate of the target.
     * @param y The global Y coordinate of the target.
     * @return The properly initialized FlyTargetingPacketClient.
     */
    static FlyTargetingPacketClient BuildSendFlyTargeting(uint32_t targetId, int32_t x, int32_t y);

    /**
     * @brief Constructs a client-side packet to add an additional fly target.
     * @param targetId The ID of the targeted character instance.
     * @param x The global X coordinate of the target.
     * @param y The global Y coordinate of the target.
     * @return The properly initialized FlyTargetingPacketClient.
     */
    static FlyTargetingPacketClient BuildSendAddFlyTargeting(uint32_t targetId, int32_t x, int32_t y);
};
