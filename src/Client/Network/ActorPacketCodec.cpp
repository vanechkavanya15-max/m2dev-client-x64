#include "../../EterBase/StdAfx.h"
#include "ActorPacketCodec.h"
#include <cstring>
#include <expected>

namespace Client::Network {

    EterBase::Result<ActorSpawnData, EterBase::PacketError> ActorPacketCodec::DecodeCharacterAdd(const TPacketGCCharacterAdd& packet) {
        // Minimal header validation or opcode check can be added if opcodes are injected,
        // but typically the codec assumes the packet struct is already cast/received.
        
        ActorSpawnData data{};
        data.vid = EntityVid(packet.dwVID);
        data.race = RaceVnum(packet.wRaceNum);
        data.x = packet.x;
        data.y = packet.y;
        data.z = packet.z;
        data.angle = packet.angle;
        data.type = packet.bType;
        data.stateFlag = packet.bStateFlag;
        data.affectFlag[0] = packet.bAffectFlag[0];
        data.affectFlag[1] = packet.bAffectFlag[1];
        
        data.hasAdditionalInfo = false;

        return data;
    }

    EterBase::Result<ActorSpawnData, EterBase::PacketError> ActorPacketCodec::DecodeCharacterAdditionalInfo(const TPacketGCCharacterAdditionalInfo& packet, ActorSpawnData existingData) {
        if (existingData.vid.value() != packet.dwVID) {
            return std::unexpected(EterBase::PacketError::SequenceMismatch);
        }

        // Properly read null-terminated string safely
        char safeName[CHARACTER_NAME_MAX_LEN + 1];
        std::memcpy(safeName, packet.name, CHARACTER_NAME_MAX_LEN + 1);
        safeName[CHARACTER_NAME_MAX_LEN] = '\0';

        existingData.name = safeName;
        existingData.parts[0] = packet.awPart[0];
        existingData.parts[1] = packet.awPart[1];
        existingData.parts[2] = packet.awPart[2];
        existingData.parts[3] = packet.awPart[3];
        existingData.empire = packet.bEmpire;
        existingData.guildId = EterBase::GuildId(packet.dwGuildID);
        existingData.level = EterBase::PlayerLevel(static_cast<uint8_t>(packet.dwLevel));
        existingData.alignment = packet.sAlignment;
        existingData.pkMode = packet.bPKMode;
        existingData.mountVnum = EterBase::ItemVnum(packet.dwMountVnum);

        existingData.hasAdditionalInfo = true;

        return existingData;
    }

    EterBase::Result<ActorSpawnData, EterBase::PacketError> ActorPacketCodec::DecodeCharacterUpdate(const TPacketGCCharacterUpdate& packet, ActorSpawnData existingData) {
        if (existingData.vid.value() != packet.dwVID) {
            return std::unexpected(EterBase::PacketError::SequenceMismatch);
        }

        existingData.parts[0] = packet.awPart[0];
        existingData.parts[1] = packet.awPart[1];
        existingData.parts[2] = packet.awPart[2];
        existingData.parts[3] = packet.awPart[3];
        existingData.stateFlag = packet.bStateFlag;
        existingData.affectFlag[0] = packet.bAffectFlag[0];
        existingData.affectFlag[1] = packet.bAffectFlag[1];
        existingData.guildId = EterBase::GuildId(packet.dwGuildID);
        existingData.alignment = packet.sAlignment;
        existingData.pkMode = packet.bPKMode;
        existingData.mountVnum = EterBase::ItemVnum(packet.dwMountVnum);

        return existingData;
    }

    EterBase::Result<EntityVid, EterBase::PacketError> ActorPacketCodec::DecodeCharacterDelete(const TPacketGCCharacterDelete& packet) {
        return EntityVid(packet.dwVID);
    }

} // namespace Client::Network
