#include "../../UserInterface/StdAfx.h"
#include "VirtualPacketGenerator.h"

#include <cstring>
#include <array>

namespace Client::Simulation
{
    namespace
    {
        constexpr uint16_t HEADER_GC_CHARACTER_ADD = 1;
        constexpr uint16_t HEADER_GC_CHARACTER_DEL = 2;
        constexpr uint16_t HEADER_GC_CHARACTER_MOVE = 3;
        constexpr uint16_t HEADER_GC_PLAYER_POINTS = 16;
        constexpr uint16_t HEADER_GC_ITEM_GROUND_ADD = 26;
        constexpr uint16_t HEADER_GC_ITEM_GROUND_DEL = 27;
        constexpr uint16_t HEADER_GC_DAMAGE_INFO = 135;

#pragma pack(push, 1)

        struct TPacketGCCharacterAdd
        {
            uint16_t header;
            uint16_t length;
            uint32_t dwVID;
            float angle;
            int32_t x;
            int32_t y;
            int32_t z;
            uint8_t bType;
            uint16_t wRaceNum;
            uint8_t bMovingSpeed;
            uint8_t bAttackSpeed;
            uint8_t bStateFlag;
            uint32_t dwAffectFlag[2];
        };
        static_assert(sizeof(TPacketGCCharacterAdd) == 38, "TPacketGCCharacterAdd layout mismatch");

        struct TPacketGCMove
        {
            uint16_t header;
            uint16_t length;
            uint8_t bFunc;
            uint8_t bArg;
            uint8_t bRot;
            uint32_t dwVID;
            int32_t lX;
            int32_t lY;
            uint32_t dwTime;
            uint32_t dwDuration;
        };
        static_assert(sizeof(TPacketGCMove) == 27, "TPacketGCMove layout mismatch");

        struct TPacketGCCharacterDelete
        {
            uint16_t header;
            uint16_t length;
            uint32_t dwVID;
        };
        static_assert(sizeof(TPacketGCCharacterDelete) == 8, "TPacketGCCharacterDelete layout mismatch");

        struct TPacketGCDamageInfo
        {
            uint16_t header;
            uint16_t length;
            uint32_t dwVID;
            uint8_t flag;
            int32_t damage;
        };
        static_assert(sizeof(TPacketGCDamageInfo) == 13, "TPacketGCDamageInfo layout mismatch");

        struct TPacketGCItemGroundAdd
        {
            uint16_t header;
            uint16_t length;
            int32_t lX;
            int32_t lY;
            int32_t lZ;
            uint32_t dwVID;
            uint32_t dwVnum;
        };
        static_assert(sizeof(TPacketGCItemGroundAdd) == 24, "TPacketGCItemGroundAdd layout mismatch");

        struct TPacketGCItemGroundDel
        {
            uint16_t header;
            uint16_t length;
            uint32_t vid;
        };
        static_assert(sizeof(TPacketGCItemGroundDel) == 8, "TPacketGCItemGroundDel layout mismatch");

        struct TPacketGCPoints
        {
            uint16_t header;
            uint16_t length;
            int32_t points[POINT_MAX_NUM];
        };
        static_assert(sizeof(TPacketGCPoints) == 1024, "TPacketGCPoints layout mismatch");

#pragma pack(pop)

        template <typename T>
        std::vector<uint8_t> SerializePacket(const T& packet)
        {
            std::vector<uint8_t> buffer(sizeof(T));
            std::memcpy(buffer.data(), &packet, sizeof(T));
            return buffer;
        }

    } // anonymous namespace

    std::vector<uint8_t> VirtualPacketGenerator::CreateActorSpawnPacket(uint32_t vid, uint32_t vnum, int32_t x, int32_t y, int32_t hp, int32_t maxHp, int32_t moveSpeed)
    {
        TPacketGCCharacterAdd packet{};
        packet.header = HEADER_GC_CHARACTER_ADD;
        packet.length = sizeof(TPacketGCCharacterAdd);
        packet.dwVID = vid;
        packet.wRaceNum = static_cast<uint16_t>(vnum);
        packet.x = x;
        packet.y = y;
        packet.z = 0;
        packet.angle = 0.0f;
        packet.bType = 0; // Default type, would map to NPC/Monster based on context
        packet.bMovingSpeed = static_cast<uint8_t>(moveSpeed);
        packet.bAttackSpeed = 100;
        packet.bStateFlag = 0;
        packet.dwAffectFlag[0] = 0;
        packet.dwAffectFlag[1] = 0;

        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreateActorMovePacket(uint32_t vid, int32_t curX, int32_t curY, int32_t destX, int32_t destY, uint8_t moveType)
    {
        TPacketGCMove packet{};
        packet.header = HEADER_GC_CHARACTER_MOVE;
        packet.length = sizeof(TPacketGCMove);
        packet.bFunc = moveType; // Typically FUNC_MOVE or similar
        packet.bArg = 0;
        packet.bRot = 0;
        packet.dwVID = vid;
        packet.lX = destX; // Using destination coordinates based on typical move packet usage
        packet.lY = destY;
        packet.dwTime = 0;
        packet.dwDuration = 0;

        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreateActorDeletePacket(uint32_t vid)
    {
        TPacketGCCharacterDelete packet{};
        packet.header = HEADER_GC_CHARACTER_DEL;
        packet.length = sizeof(TPacketGCCharacterDelete);
        packet.dwVID = vid;
        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreateDamageInfoPacket(uint32_t attackerVid, uint32_t victimVid, int32_t damage, uint8_t damageFlag)
    {
        TPacketGCDamageInfo packet{};
        packet.header = HEADER_GC_DAMAGE_INFO;
        packet.length = sizeof(TPacketGCDamageInfo);
        packet.dwVID = victimVid;
        packet.flag = damageFlag;
        packet.damage = damage;
        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreateItemGroundAddPacket(uint32_t itemVid, uint32_t vnum, int32_t x, int32_t y, uint32_t ownerVid)
    {
        TPacketGCItemGroundAdd packet{};
        packet.header = HEADER_GC_ITEM_GROUND_ADD;
        packet.length = sizeof(TPacketGCItemGroundAdd);
        packet.dwVID = itemVid;
        packet.dwVnum = vnum;
        packet.lX = x;
        packet.lY = y;
        packet.lZ = 0;

        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreateItemGroundDelPacket(uint32_t itemVid)
    {
        TPacketGCItemGroundDel packet{};
        packet.header = HEADER_GC_ITEM_GROUND_DEL;
        packet.length = sizeof(TPacketGCItemGroundDel);
        packet.vid = itemVid;
        return SerializePacket(packet);
    }

    std::vector<uint8_t> VirtualPacketGenerator::CreatePlayerPointsPacket(const std::map<uint8_t, int32_t>& pointsMap)
    {
        TPacketGCPoints packet{};
        packet.header = HEADER_GC_PLAYER_POINTS;
        packet.length = sizeof(TPacketGCPoints);
        std::memset(packet.points, 0, sizeof(packet.points));

        for (const auto& [pointType, value] : pointsMap)
        {
            if (pointType < POINT_MAX_NUM)
            {
                packet.points[pointType] = value;
            }
        }
        return SerializePacket(packet);
    }
} // namespace Client::Simulation
