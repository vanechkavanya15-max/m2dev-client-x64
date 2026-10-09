#include "ActorUpdatePacketHandler.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<ActorVisualUpdateData> ActorUpdatePacketHandler::HandleCharacterUpdate(std::span<const uint8_t> payload) noexcept {
    if (payload.size() < sizeof(TPacketGCCharacterUpdate)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCharacterUpdate packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCharacterUpdate));

    if (packet.dwVID == 0) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    ActorVisualUpdateData data{};
    data.vid = EterBase::EntityId(packet.dwVID);
    
    for (size_t i = 0; i < CHR_EQUIPPART_NUM; ++i) {
        data.parts[i] = packet.awPart[i];
    }
    
    data.movingSpeed = packet.bMovingSpeed;
    data.attackSpeed = packet.bAttackSpeed;
    data.stateFlag = packet.bStateFlag;
    data.affectFlags[0] = packet.dwAffectFlag[0];
    data.affectFlags[1] = packet.dwAffectFlag[1];
    data.guildId = packet.dwGuildID;
    data.alignment = packet.sAlignment;
    data.pkMode = packet.bPKMode;
    data.mountVnum = packet.dwMountVnum;

    return data;
}

EterBase::PacketResult<ActorVisualUpdateData> ActorUpdatePacketHandler::HandleCharacterUpdate2(std::span<const uint8_t> payload) noexcept {
    if (payload.size() < sizeof(TPacketGCCharacterUpdate2)) {
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCharacterUpdate2 packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCharacterUpdate2));

    if (packet.dwVID == 0) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    ActorVisualUpdateData data{};
    data.vid = EterBase::EntityId(packet.dwVID);
    
    for (size_t i = 0; i < CHR_EQUIPPART_NUM; ++i) {
        data.parts[i] = packet.awPart[i];
    }
    
    data.movingSpeed = packet.bMovingSpeed;
    data.attackSpeed = packet.bAttackSpeed;
    data.stateFlag = packet.bStateFlag;
    data.affectFlags[0] = packet.dwAffectFlag[0];
    data.affectFlags[1] = packet.dwAffectFlag[1];
    data.guildId = packet.dwGuildID;
    data.alignment = packet.sAlignment;
    data.pkMode = packet.bPKMode;
    data.mountVnum = packet.dwMountVnum;

    return data;
}

} // namespace Client::Network::Handlers
