
#include "SelectPacketHandler.h"
#include "../Protocol/Protocol.h"
#include <cstring>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> SelectPacketHandler::HandleLoginSuccess4(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCLoginSuccess4)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCLoginSuccess4 packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCLoginSuccess4));

    for (uint8_t i = 0; i < PLAYER_PER_ACCOUNT4; ++i) {
        const auto& info = packet.akSimplePlayerInformation[i];
        
        if (info.dwID != 0) {
            CharacterSlotData data;
            data.id = info.dwID;
            
            char nameBuf[CHARACTER_NAME_MAX_LEN + 1] = {0};
            std::memcpy(nameBuf, info.szName, CHARACTER_NAME_MAX_LEN);
            data.name = nameBuf;
            
            data.job = info.byJob;
            data.level = info.byLevel;
            data.playMinutes = info.dwPlayMinutes;
            data.str = info.byST;
            data.ht = info.byHT;
            data.dex = info.byDX;
            data.iq = info.byIQ;
            data.mainPart = info.wMainPart;
            data.hairPart = info.wHairPart;
            data.skillGroup = info.bySkillGroup;
            data.changeName = info.bChangeName != 0;
            data.slotIndex = i;
            data.guildId = packet.guild_id[i];

            char guildNameBuf[GUILD_NAME_MAX_LEN + 1] = {0};
            std::memcpy(guildNameBuf, packet.guild_name[i], GUILD_NAME_MAX_LEN);
            data.guildName = guildNameBuf;

            Client::Core::EventBus::Instance().Publish(CharacterSlotUpdatedEvent(data));
        }
    }

    return {};
}

EterBase::PacketResult<void> SelectPacketHandler::HandleCreateSuccess(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPlayerCreateSuccess)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPlayerCreateSuccess packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPlayerCreateSuccess));

    if (packet.bAccountCharacterSlot >= PLAYER_PER_ACCOUNT4) {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    const auto& info = packet.kSimplePlayerInfomation;
    
    CharacterSlotData data;
    data.id = info.dwID;
    
    char nameBuf[CHARACTER_NAME_MAX_LEN + 1] = {0};
    std::memcpy(nameBuf, info.szName, CHARACTER_NAME_MAX_LEN);
    data.name = nameBuf;
    
    data.job = info.byJob;
    data.level = info.byLevel;
    data.playMinutes = info.dwPlayMinutes;
    data.str = info.byST;
    data.ht = info.byHT;
    data.dex = info.byDX;
    data.iq = info.byIQ;
    data.mainPart = info.wMainPart;
    data.hairPart = info.wHairPart;
    data.skillGroup = info.bySkillGroup;
    data.changeName = info.bChangeName != 0;
    data.slotIndex = packet.bAccountCharacterSlot;
    data.guildId = 0; // New character has no guild
    data.guildName = "";

    Client::Core::EventBus::Instance().Publish(CharacterSlotUpdatedEvent(data));

    return {};
}

EterBase::PacketResult<void> SelectPacketHandler::HandleDeleteSuccess(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCDestroyCharacterSuccess)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCDestroyCharacterSuccess packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCDestroyCharacterSuccess));

    if (packet.account_index >= PLAYER_PER_ACCOUNT4) {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    Client::Core::EventBus::Instance().Publish(CharacterSlotDeletedEvent(packet.account_index));

    return {};
}

} // namespace Client::Network::Handlers
