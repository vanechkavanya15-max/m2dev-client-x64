#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "../../../EterBase/Result.h"
#include "../../Gameplay/SocialDomain.h"
#include "../Protocol/Protocol.h"

namespace Client::Network::Handlers {

/**
 * Obsluguje pakiety informacji o gildii (GuildInfo, GuildGrade, GuildMember, GuildSkill).
 * Deserializuje pakiety GUILD (podnaglowki INFO, GRADE, SKILL_INFO, LIST) i aktualizuje SocialManager.
 */
class GuildInfoPacketHandler {
public:
    explicit GuildInfoPacketHandler(Client::Gameplay::SocialManager& socialManager);

    /**
     * Parsuje podnaglowek GuildSub::GC::INFO (TPacketGCGuildInfo).
     */
    EterBase::PacketResult<void> HandleGuildInfoPacket(std::span<const uint8_t> payload);

    /**
     * Parsuje podnaglowek GuildSub::GC::GRADE (tablica TPacketGCGuildSubGrade).
     */
    EterBase::PacketResult<void> HandleGuildGradePacket(std::span<const uint8_t> payload);

    /**
     * Parsuje podnaglowek GuildSub::GC::LIST (czlonkowie gildii - TPacketGCGuildSubMember).
     */
    EterBase::PacketResult<void> HandleGuildMemberPacket(std::span<const uint8_t> payload);

    /**
     * Parsuje podnaglowek GuildSub::GC::SKILL_INFO (umiejetnosci gildyjne).
     */
    EterBase::PacketResult<void> HandleGuildSkillPacket(std::span<const uint8_t> payload);

private:
    Client::Gameplay::SocialManager& m_socialManager;
};

struct GuildGradeData {
    std::string name;
    uint8_t authFlag;
    
    GuildGradeData(const std::string& n, uint8_t a) : name(n), authFlag(a) {}
};

// Zdarzenia dla EventBus (architektura bez zaleznosci/decoupling)
struct GuildGradeUpdatedEvent {
    std::vector<GuildGradeData> grades;
    explicit GuildGradeUpdatedEvent(const std::vector<GuildGradeData>& g) : grades(g) {}
    explicit GuildGradeUpdatedEvent(std::vector<GuildGradeData>&& g) : grades(std::move(g)) {}
};

struct GuildSkillUpdatedEvent {
    std::vector<TPlayerSkill> skills;
    explicit GuildSkillUpdatedEvent(std::vector<TPlayerSkill>&& s) : skills(std::move(s)) {}
};

} // namespace Client::Network::Handlers
