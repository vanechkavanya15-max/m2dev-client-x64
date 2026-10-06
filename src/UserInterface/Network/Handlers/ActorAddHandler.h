#pragma once

#include <cstdint>
#include <span>

class CNetworkActorManager;

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Structure representing the actor addition packet payload from the server.
     */
    struct ActorAddPacket
    {
        uint8_t  header;        ///< Packet identifier (e.g., 0x01)
        uint32_t id;            ///< Unique virtual ID of the actor
        float    angle;         ///< Facing angle/rotation
        int32_t  x;             ///< Global X coordinate
        int32_t  y;             ///< Global Y coordinate
        int32_t  z;             ///< Global Z coordinate
        uint8_t  type;          ///< Type of the actor (player, npc, mob)
        uint16_t raceNum;       ///< VNUM (model ID) or character class
        uint8_t  movingSpeed;   ///< Movement speed attribute
        uint8_t  attackSpeed;   ///< Attack speed attribute
        uint8_t  stateFlag;     ///< Superimposed states (e.g., bits for attack, move)
        uint32_t affectFlag[2]; ///< Mask of status effects (e.g., poison, stun)
        uint8_t  reserved[10];  ///< Reserved padding bytes
    };
#pragma pack(pop)

    /**
     * @brief Handles the incoming payload for adding a new actor to the game world.
     * 
     * @param payload Binary view representing the incoming network packet.
     * @param actorManager Reference to the central actor manager for registration.
     * @return true if the packet was successfully parsed and applied; otherwise false.
     */
    bool HandleActorAdd(std::span<const uint8_t> payload, CNetworkActorManager& actorManager);
}
