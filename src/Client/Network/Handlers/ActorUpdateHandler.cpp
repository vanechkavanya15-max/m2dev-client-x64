#include "ActorUpdateHandler.h"
#include "../../../UserInterface/Packet.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<ActorVisualUpdateData> ActorUpdateHandler::HandleActorUpdate(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCCharacterUpdate)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCCharacterUpdate packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCCharacterUpdate));

    if (packet.dwVID == 0) {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    ActorVisualUpdateData data;
    data.vid = EterBase::EntityId(packet.dwVID);
    data.armorVnum = packet.awPart[CHR_EQUIPPART_ARMOR];
    data.weaponVnum = packet.awPart[CHR_EQUIPPART_WEAPON];
    data.hairVnum = packet.awPart[CHR_EQUIPPART_HAIR];
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
