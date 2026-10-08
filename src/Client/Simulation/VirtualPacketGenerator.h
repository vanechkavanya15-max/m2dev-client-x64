#pragma once

#include <vector>
#include <cstdint>
#include <map>

namespace Client::Simulation
{
    /**
     * @brief Synthetic Virtual Packet Generator for Metin2 C++23 Client Modernization.
     * Generates byte-aligned raw payload vectors for mocked Server-to-Client (GC) network packets.
     * Ensures strict adherence to original structs and Zero-Conflict by acting as an isolated layer.
     */
    class VirtualPacketGenerator
    {
    public:
        VirtualPacketGenerator() = delete;
        ~VirtualPacketGenerator() = delete;

        static std::vector<uint8_t> CreateActorSpawnPacket(uint32_t vid, uint32_t vnum, int32_t x, int32_t y, int32_t hp, int32_t maxHp, int32_t moveSpeed);
        static std::vector<uint8_t> CreateActorMovePacket(uint32_t vid, int32_t curX, int32_t curY, int32_t destX, int32_t destY, uint8_t moveType);
        static std::vector<uint8_t> CreateActorDeletePacket(uint32_t vid);
        static std::vector<uint8_t> CreateDamageInfoPacket(uint32_t attackerVid, uint32_t victimVid, int32_t damage, uint8_t damageFlag);
        static std::vector<uint8_t> CreateItemGroundAddPacket(uint32_t itemVid, uint32_t vnum, int32_t x, int32_t y, uint32_t ownerVid);
        static std::vector<uint8_t> CreateItemGroundDelPacket(uint32_t itemVid);
        static std::vector<uint8_t> CreatePlayerPointsPacket(const std::map<uint8_t, int32_t>& pointsMap);
    };
} // namespace Client::Simulation
