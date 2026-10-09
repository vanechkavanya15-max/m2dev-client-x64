#include "StdAfx.h"
#include "GuildInfoPacketHandler.h"
#include "../Protocol/Protocol.h"
#include "Client/Core/EventBus.h"
#include <cstring>
#include <memory>

namespace Client::Network::Handlers {

GuildInfoPacketHandler::GuildInfoPacketHandler(Client::Gameplay::SocialManager& socialManager)
    : m_socialManager(socialManager) {}

EterBase::PacketResult<void> GuildInfoPacketHandler::HandleGuildInfoPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCGuildInfo)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCGuildInfo*>(payload.data());
    
    std::string guildName(packet->name, strnlen(packet->name, GUILD_NAME_MAX_LEN));
    
    // Zakladamy range 1 dla poczatkowego tworzenia, 
    // poniewaz INFO nie dostarcza bezposrednio rangi.
    auto guild = std::make_shared<Client::Gameplay::Guild>(
        EterBase::GuildId{packet->guild_id}, 
        guildName, 
        1
    );

    guild->SetBank(packet->gold);
    guild->SetExp(packet->level, packet->exp);

    m_socialManager.SetGuild(guild);

    return {};
}

EterBase::PacketResult<void> GuildInfoPacketHandler::HandleGuildGradePacket(std::span<const uint8_t> payload) {
    constexpr size_t expectedSize = sizeof(TPacketGCGuildSubGrade) * GUILD_GRADE_COUNT;
    
    if (payload.size() < expectedSize) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    std::vector<GuildGradeData> grades;
    grades.reserve(GUILD_GRADE_COUNT);

    const auto* gradesArray = reinterpret_cast<const TPacketGCGuildSubGrade*>(payload.data());
    for (size_t i = 0; i < GUILD_GRADE_COUNT; ++i) {
        const auto& gradePack = gradesArray[i];
        std::string gradeName(gradePack.grade_name, strnlen(gradePack.grade_name, GUILD_GRADE_NAME_MAX_LEN));
        grades.emplace_back(gradeName, gradePack.auth_flag);
    }

    Client::Core::EventBus::GetInstance().Publish(GuildGradeUpdatedEvent(std::move(grades)));

    return {};
}

EterBase::PacketResult<void> GuildInfoPacketHandler::HandleGuildMemberPacket(std::span<const uint8_t> payload) {
    size_t offset = 0;
    size_t remainingSize = payload.size();

    while (remainingSize > 0) {
        if (remainingSize < sizeof(TPacketGCGuildSubMember)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        
        const auto* memberPack = reinterpret_cast<const TPacketGCGuildSubMember*>(payload.data() + offset);
        offset += sizeof(TPacketGCGuildSubMember);
        remainingSize -= sizeof(TPacketGCGuildSubMember);

        std::string memberName = "";
        
        // Jesli byNameFlag jest wlaczone, serwer wysyla dodatkowo stringa z nazwa postaci
        if (memberPack->byNameFlag) {
            constexpr size_t nameLen = CHARACTER_NAME_MAX_LEN + 1;
            if (remainingSize < nameLen) {
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }
            char nameBuf[nameLen];
            std::memcpy(nameBuf, payload.data() + offset, nameLen);
            nameBuf[CHARACTER_NAME_MAX_LEN] = '\0';
            memberName = nameBuf;
            
            offset += nameLen;
            remainingSize -= nameLen;
        }

        auto guild = m_socialManager.GetGuild();
        if (guild) {
            // Ustaw uprawnienia na podstawie rangi/flagi autoryzacji
            uint32_t permissions = memberPack->byGrade;
            Client::Gameplay::GuildMember newMember(EterBase::EntityId{memberPack->pid}, memberName, permissions);
            
            if (guild->GetMember(EterBase::EntityId{memberPack->pid})) {
                 (void)guild->RemoveMember(EterBase::EntityId{memberPack->pid}); // Odswiez
            }
            (void)guild->AddMember(newMember);
        }
    }

    return {};
}

EterBase::PacketResult<void> GuildInfoPacketHandler::HandleGuildSkillPacket(std::span<const uint8_t> payload) {
    // Serwer wysyla struktury TPlayerSkill
    size_t skillCount = payload.size() / sizeof(TPlayerSkill);
    
    if (skillCount == 0 || payload.size() % sizeof(TPlayerSkill) != 0) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    std::vector<TPlayerSkill> skills;
    skills.reserve(skillCount);

    const auto* skillArray = reinterpret_cast<const TPlayerSkill*>(payload.data());
    for (size_t i = 0; i < skillCount; ++i) {
        skills.push_back(skillArray[i]);
    }

    Client::Core::EventBus::GetInstance().Publish(GuildSkillUpdatedEvent(std::move(skills)));

    return {};
}

} // namespace Client::Network::Handlers
