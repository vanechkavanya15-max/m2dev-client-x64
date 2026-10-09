#include "GuildPacketDomainHandler.h"
#include "EterBase/LogModern.h"
#include <cstring>
#include <algorithm>

namespace Client::Network::Handlers {

GuildPacketDomainHandler::GuildPacketDomainHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager)
    : m_socialManager(std::move(socialManager)) {
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleGuildPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCGuild)) {
        EterBase::ModernLogger::Error("GuildPacketDomainHandler: Bufor TPacketGCGuild za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCGuild));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* pack = reinterpret_cast<const TPacketGCGuild*>(payload.data());
    std::span<const uint8_t> subPayload = payload.subspan(sizeof(TPacketGCGuild));

    switch (pack->subheader) {
        case GuildSub::GC::LOGIN:
            return HandleSubLogin(subPayload);
        case GuildSub::GC::LOGOUT:
            return HandleSubLogout(subPayload);
        case GuildSub::GC::REMOVE:
            return HandleSubRemove(subPayload);
        case GuildSub::GC::LIST:
            return HandleSubList(subPayload);
        case GuildSub::GC::GRADE:
            return HandleSubGrade(subPayload);
        case GuildSub::GC::GRADE_NAME:
            return HandleSubGradeName(subPayload);
        case GuildSub::GC::GRADE_AUTH:
            return HandleSubGradeAuth(subPayload);
        case GuildSub::GC::INFO:
            return HandleSubInfo(subPayload);
        case GuildSub::GC::COMMENTS:
            return HandleSubComments(subPayload);
        case GuildSub::GC::CHANGE_EXP:
            return HandleSubChangeExp(subPayload);
        case GuildSub::GC::CHANGE_MEMBER_GRADE:
            return HandleSubChangeMemberGrade(subPayload);
        case GuildSub::GC::SKILL_INFO:
            return HandleSubSkillInfo(subPayload);
        case GuildSub::GC::CHANGE_MEMBER_GENERAL:
            return HandleSubChangeMemberGeneral(subPayload);
        case GuildSub::GC::GUILD_INVITE:
            return HandleSubInvite(subPayload);
        case GuildSub::GC::WAR:
            return HandleSubWar(subPayload);
        case GuildSub::GC::WAR_POINT:
            return HandleSubWarPoint(subPayload);
        case GuildSub::GC::MONEY_CHANGE:
            return HandleSubMoneyChange(subPayload);
        default:
            EterBase::ModernLogger::Warning("GuildPacketDomainHandler: Nieznany subheader gildii: 0x{:02X}", pack->subheader);
            return {};
    }
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleRequestMakeGuild(std::span<const uint8_t> payload) {
    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Otrzymano zapytanie o utworzenie nowej gildii");
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubLogin(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t pid = 0;
    std::memcpy(&pid, payload.data(), sizeof(uint32_t));

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Członek PID: {} zalogował się", pid);
    Client::Core::Events::GuildMemberLoginEvent event(pid);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubLogout(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t pid = 0;
    std::memcpy(&pid, payload.data(), sizeof(uint32_t));

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Członek PID: {} wylogował się", pid);
    Client::Core::Events::GuildMemberLogoutEvent event(pid);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubRemove(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t pid = 0;
    std::memcpy(&pid, payload.data(), sizeof(uint32_t));

    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Usunięto członka PID: {} z gildii", pid);
    Client::Core::Events::GuildMemberRemoveEvent event(pid);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubList(std::span<const uint8_t> payload) {
    size_t offset = 0;
    while (offset + sizeof(TPacketGCGuildSubMember) <= payload.size()) {
        TPacketGCGuildSubMember member{};
        std::memcpy(&member, payload.data() + offset, sizeof(TPacketGCGuildSubMember));
        offset += sizeof(TPacketGCGuildSubMember);

        std::string memberName;
        if (member.byNameFlag) {
            if (offset + CHARACTER_NAME_MAX_LEN + 1 <= payload.size()) {
                const char* namePtr = reinterpret_cast<const char*>(payload.data() + offset);
                memberName = std::string(namePtr, strnlen(namePtr, CHARACTER_NAME_MAX_LEN));
                offset += CHARACTER_NAME_MAX_LEN + 1;
            }
        }

        Client::Core::Events::GuildMemberInfoSyncEvent event(
            member.pid, memberName, member.byGrade, member.byJob, member.byLevel, member.dwOffer, member.byIsGeneral
        );
        Client::Core::EventBus::GetInstance().Publish(event);
    }
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubGrade(std::span<const uint8_t> payload) {
    if (payload.empty()) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint8_t count = payload[0];
    size_t offset = 1;
    for (uint8_t i = 0; i < count && offset + 1 + sizeof(TPacketGCGuildSubGrade) <= payload.size(); ++i) {
        offset += 1 + sizeof(TPacketGCGuildSubGrade);
    }
    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Zsynchronizowano {} rang gildii", count);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubGradeName(std::span<const uint8_t> payload) {
    if (payload.size() < 1 + GUILD_GRADE_NAME_MAX_LEN + 1) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint8_t gradeNumber = payload[0];
    const char* namePtr = reinterpret_cast<const char*>(payload.data() + 1);
    std::string name(namePtr, strnlen(namePtr, GUILD_GRADE_NAME_MAX_LEN));

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Zmieniono nazwę rangi {}: {}", gradeNumber, name);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubGradeAuth(std::span<const uint8_t> payload) {
    if (payload.size() < 2) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint8_t gradeNumber = payload[0];
    uint8_t authFlag = payload[1];

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Zmieniono uprawnienia rangi {}: 0x{:02X}", gradeNumber, authFlag);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubInfo(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCGuildInfo)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    TPacketGCGuildInfo info{};
    std::memcpy(&info, payload.data(), sizeof(TPacketGCGuildInfo));

    std::string guildName(info.name, strnlen(info.name, sizeof(info.name)));
    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Synchronizacja gildii {}: poziom {}, exp {}, zloto {}",
        guildName, info.level, info.exp, info.gold);

    Client::Core::Events::GuildInfoSyncEvent event(
        info.guild_id, guildName, info.master_pid, info.level, info.exp, info.gold, info.max_member_count
    );
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubComments(std::span<const uint8_t> payload) {
    if (payload.empty()) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint8_t count = payload[0];
    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Otrzymano {} ogłoszeń gildii", count);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubChangeExp(std::span<const uint8_t> payload) {
    if (payload.size() < 1 + sizeof(uint32_t)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint8_t level = payload[0];
    uint32_t exp = 0;
    std::memcpy(&exp, payload.data() + 1, sizeof(uint32_t));

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Aktualizacja exp gildii: poziom {}, exp {}", level, exp);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubChangeMemberGrade(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t) + 1) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t pid = 0;
    std::memcpy(&pid, payload.data(), sizeof(uint32_t));
    uint8_t grade = payload[sizeof(uint32_t)];

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Zmiana rangi członka PID: {} na {}", pid, grade);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubSkillInfo(std::span<const uint8_t> payload) {
    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Otrzymano dane umiejętności gildyjnych");
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubChangeMemberGeneral(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t) + 1) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t pid = 0;
    std::memcpy(&pid, payload.data(), sizeof(uint32_t));
    uint8_t flag = payload[sizeof(uint32_t)];

    EterBase::ModernLogger::Debug("GuildPacketDomainHandler: Zmiana flagi generala PID: {} na {}", pid, flag);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubInvite(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t) + GUILD_NAME_MAX_LEN) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t guildId = 0;
    std::memcpy(&guildId, payload.data(), sizeof(uint32_t));
    const char* namePtr = reinterpret_cast<const char*>(payload.data() + sizeof(uint32_t));
    std::string guildName(namePtr, strnlen(namePtr, GUILD_NAME_MAX_LEN));

    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Zaproszenie do gildii {} (ID: {})", guildName, guildId);
    Client::Core::Events::GuildInviteReceivedEvent event(guildId, guildName);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubWar(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCGuildWar)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    TPacketGCGuildWar war{};
    std::memcpy(&war, payload.data(), sizeof(TPacketGCGuildWar));

    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Stan wojny gildii: Self {}, Opp {}, Typ {}, Stan {}",
        war.dwGuildSelf, war.dwGuildOpp, war.bType, war.bWarState);

    Client::Core::Events::GuildWarEvent event(war.dwGuildSelf, war.dwGuildOpp, war.bType, war.bWarState);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubWarPoint(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGuildWarPoint)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    TPacketGuildWarPoint pt{};
    std::memcpy(&pt, payload.data(), sizeof(TPacketGuildWarPoint));

    Client::Core::Events::GuildWarPointEvent event(pt.dwGainGuildID, pt.dwOpponentGuildID, pt.lPoint);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

EterBase::PacketResult<void> GuildPacketDomainHandler::HandleSubMoneyChange(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(uint32_t)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }
    uint32_t gold = 0;
    std::memcpy(&gold, payload.data(), sizeof(uint32_t));

    EterBase::ModernLogger::Info("GuildPacketDomainHandler: Zmiana stanu skarbca gildii: {} Yang", gold);
    Client::Core::Events::GuildMoneyChangeEvent event(gold);
    Client::Core::EventBus::GetInstance().Publish(event);
    return {};
}

// Outbound Encoders
std::vector<uint8_t> GuildPacketDomainHandler::EncodeAddMember(uint32_t vid) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::ADD_MEMBER;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &vid, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeRemoveMember(uint32_t pid) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::REMOVE_MEMBER;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &pid, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeChangeGradeName(uint8_t grade, std::string_view name) {
    char szName[GUILD_GRADE_NAME_MAX_LEN + 1] = {};
    std::memcpy(szName, name.data(), std::min(name.size(), static_cast<size_t>(GUILD_GRADE_NAME_MAX_LEN)));

    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + 1 + sizeof(szName);
    header.bySubHeader = GuildSub::CG::CHANGE_GRADE_NAME;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    buffer[sizeof(TPacketCGGuild)] = grade;
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild) + 1, szName, sizeof(szName));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeChangeGradeAuthority(uint8_t grade, uint8_t authority) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + 2;
    header.bySubHeader = GuildSub::CG::CHANGE_GRADE_AUTHORITY;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    buffer[sizeof(TPacketCGGuild)] = grade;
    buffer[sizeof(TPacketCGGuild) + 1] = authority;
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeOfferExp(uint32_t exp) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::OFFER;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &exp, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodePostComment(std::string_view message) {
    uint8_t bySize = static_cast<uint8_t>(std::min(message.size() + 1, static_cast<size_t>(GULID_COMMENT_MAX_LEN + 1)));
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + 1 + bySize;
    header.bySubHeader = GuildSub::CG::POST_COMMENT;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    buffer[sizeof(TPacketCGGuild)] = bySize;
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild) + 1, message.data(), bySize - 1);
    buffer.back() = '\0';
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeDeleteComment(uint32_t index) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::DELETE_COMMENT;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &index, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeRefreshComments() {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild);
    header.bySubHeader = GuildSub::CG::REFRESH_COMMENT;

    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeChangeMemberGrade(uint32_t pid, uint8_t grade) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t) + 1;
    header.bySubHeader = GuildSub::CG::CHANGE_MEMBER_GRADE;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &pid, sizeof(uint32_t));
    buffer[sizeof(TPacketCGGuild) + sizeof(uint32_t)] = grade;
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeUseSkill(uint32_t skillId, uint32_t targetVid) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t) * 2;
    header.bySubHeader = GuildSub::CG::USE_SKILL;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &skillId, sizeof(uint32_t));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild) + sizeof(uint32_t), &targetVid, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeChangeMemberGeneral(uint32_t pid, uint8_t isGeneral) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t) + 1;
    header.bySubHeader = GuildSub::CG::CHANGE_MEMBER_GENERAL;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &pid, sizeof(uint32_t));
    buffer[sizeof(TPacketCGGuild) + sizeof(uint32_t)] = isGeneral;
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeInviteAnswer(uint32_t guildId, uint8_t answer) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t) + 1;
    header.bySubHeader = GuildSub::CG::GUILD_INVITE_ANSWER;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &guildId, sizeof(uint32_t));
    buffer[sizeof(TPacketCGGuild) + sizeof(uint32_t)] = answer;
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeDepositMoney(uint32_t amount) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::DEPOSIT_MONEY;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &amount, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeWithdrawMoney(uint32_t amount) {
    TPacketCGGuild header{};
    header.header = CG::GUILD;
    header.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);
    header.bySubHeader = GuildSub::CG::WITHDRAW_MONEY;

    std::vector<uint8_t> buffer(header.length);
    std::memcpy(buffer.data(), &header, sizeof(TPacketCGGuild));
    std::memcpy(buffer.data() + sizeof(TPacketCGGuild), &amount, sizeof(uint32_t));
    return buffer;
}

std::vector<uint8_t> GuildPacketDomainHandler::EncodeAnswerMakeGuild(std::string_view guildName) {
    TPacketCGAnswerMakeGuild packet{};
    packet.header = CG::ANSWER_MAKE_GUILD;
    packet.length = sizeof(TPacketCGAnswerMakeGuild);
    std::memcpy(packet.guild_name, guildName.data(), std::min(guildName.size(), static_cast<size_t>(GUILD_NAME_MAX_LEN)));

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network::Handlers
