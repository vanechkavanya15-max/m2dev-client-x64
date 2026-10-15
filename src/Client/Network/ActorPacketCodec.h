#pragma once

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include <cstdint>
#include <cstddef>
#include <string>

namespace Client::Network {

    constexpr size_t CHARACTER_NAME_MAX_LEN = 64;

    struct RaceVnumTag {};
    using RaceVnum = EterBase::StrongType<RaceVnumTag, uint32_t, 0>;
    using EntityVid = EterBase::EntityId;

#pragma pack(push, 1)

    struct TPacketGCCharacterAdd {
        uint8_t header;
        uint32_t dwVID;
        float angle;
        float x;
        float y;
        float z;
        uint8_t bType;
        uint32_t wRaceNum;
        uint8_t bStateFlag;
        uint8_t bAffectFlag[2];
    };

    struct TPacketGCCharacterAdditionalInfo {
        uint8_t header;
        uint32_t dwVID;
        char name[CHARACTER_NAME_MAX_LEN + 1];
        uint32_t awPart[4];
        uint8_t bEmpire;
        uint32_t dwGuildID;
        uint32_t dwLevel;
        int16_t sAlignment;
        uint8_t bPKMode;
        uint32_t dwMountVnum;
    };

    struct TPacketGCCharacterUpdate {
        uint8_t header;
        uint32_t dwVID;
        uint32_t awPart[4];
        uint8_t bStateFlag;
        uint8_t bAffectFlag[2];
        uint32_t dwGuildID;
        int16_t sAlignment;
        uint8_t bPKMode;
        uint32_t dwMountVnum;
    };

    struct TPacketGCCharacterDelete {
        uint8_t header;
        uint32_t dwVID;
    };

#pragma pack(pop)

    struct ActorSpawnData {
        EntityVid vid;
        RaceVnum race;
        float x, y, z, angle;
        uint8_t type;
        uint8_t stateFlag;
        uint8_t affectFlag[2];
        
        // Additional Info
        std::string name;
        uint32_t parts[4];
        uint8_t empire;
        EterBase::GuildId guildId;
        EterBase::PlayerLevel level;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;
        
        bool hasAdditionalInfo = false;
    };

    class ActorPacketCodec {
    public:
        static EterBase::Result<ActorSpawnData, EterBase::PacketError> DecodeCharacterAdd(const TPacketGCCharacterAdd& packet);
        static EterBase::Result<ActorSpawnData, EterBase::PacketError> DecodeCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet, ActorSpawnData existingData);
        static EterBase::Result<ActorSpawnData, EterBase::PacketError> DecodeCharacterUpdate(const TPacketGCCharacterUpdate& packet, ActorSpawnData existingData);
        static EterBase::Result<EntityVid, EterBase::PacketError> DecodeCharacterDelete(const TPacketGCCharacterDelete& packet);
    };

} // namespace Client::Network
