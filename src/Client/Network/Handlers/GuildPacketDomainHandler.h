#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "EterBase/PacketResult.h"
#include "Client/Core/EventBus.h"
#include "Client/Gameplay/SocialDomain.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Core::Events {

/**
 * @brief Zdarzenie logowania czlonka gildii.
 */
struct GuildMemberLoginEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    constexpr explicit GuildMemberLoginEvent(uint32_t p) noexcept : pid(p) {}
};

/**
 * @brief Zdarzenie wylogowania czlonka gildii.
 */
struct GuildMemberLogoutEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    constexpr explicit GuildMemberLogoutEvent(uint32_t p) noexcept : pid(p) {}
};

/**
 * @brief Zdarzenie usuniecia czlonka gildii.
 */
struct GuildMemberRemoveEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    constexpr explicit GuildMemberRemoveEvent(uint32_t p) noexcept : pid(p) {}
};

/**
 * @brief Zdarzenie synchronizacji danych pojedynczego czlonka gildii z listy.
 */
struct GuildMemberInfoSyncEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    std::string name;
    uint8_t grade{0};
    uint8_t job{0};
    uint8_t level{0};
    uint32_t offer{0};
    uint8_t isGeneral{0};

    GuildMemberInfoSyncEvent(uint32_t p, std::string n, uint8_t g, uint8_t j, uint8_t lvl, uint32_t off, uint8_t gen)
        : pid(p), name(std::move(n)), grade(g), job(j), level(lvl), offer(off), isGeneral(gen) {}
};

/**
 * @brief Zdarzenie podstawowych informacji o gildii (poziom, doswiadczenie, zloto).
 */
struct GuildInfoSyncEvent : public Client::Core::IEvent {
    uint32_t guildId{0};
    std::string name;
    uint32_t masterPid{0};
    uint8_t level{0};
    uint32_t exp{0};
    uint32_t gold{0};
    uint32_t maxMemberCount{0};

    GuildInfoSyncEvent(uint32_t id, std::string n, uint32_t m, uint8_t lvl, uint32_t e, uint32_t g, uint32_t maxMem)
        : guildId(id), name(std::move(n)), masterPid(m), level(lvl), exp(e), gold(g), maxMemberCount(maxMem) {}
};

/**
 * @brief Zdarzenie zmiany stanu wojny gildii.
 */
struct GuildWarEvent : public Client::Core::IEvent {
    uint32_t guildSelf{0};
    uint32_t guildOpponent{0};
    uint8_t warType{0};
    uint8_t warState{0}; // SEND_DECLARE, RECV_DECLARE, ON_WAR, END

    constexpr GuildWarEvent(uint32_t self, uint32_t opp, uint8_t type, uint8_t state) noexcept
        : guildSelf(self), guildOpponent(opp), warType(type), warState(state) {}
};

/**
 * @brief Zdarzenie aktualizacji punktow wojny gildii.
 */
struct GuildWarPointEvent : public Client::Core::IEvent {
    uint32_t gainGuildId{0};
    uint32_t opponentGuildId{0};
    int32_t point{0};

    constexpr GuildWarPointEvent(uint32_t gain, uint32_t opp, int32_t pt) noexcept
        : gainGuildId(gain), opponentGuildId(opp), point(pt) {}
};

/**
 * @brief Zdarzenie zmiany stanu skarbca gildii (zloto / magazyn).
 */
struct GuildMoneyChangeEvent : public Client::Core::IEvent {
    uint32_t gold{0};
    constexpr explicit GuildMoneyChangeEvent(uint32_t g) noexcept : gold(g) {}
};

/**
 * @brief Zdarzenie otrzymania zaproszenia do gildii.
 */
struct GuildInviteReceivedEvent : public Client::Core::IEvent {
    uint32_t guildId{0};
    std::string guildName;

    GuildInviteReceivedEvent(uint32_t id, std::string name)
        : guildId(id), guildName(std::move(name)) {}
};

/**
 * @brief Zdarzenie synchronizacji danych rangi gildii.
 */
struct GuildGradeSyncEvent : public Client::Core::IEvent {
    uint8_t gradeNumber{0};
    std::string gradeName;
    uint8_t authFlag{0};

    GuildGradeSyncEvent(uint8_t num, std::string name, uint8_t auth)
        : gradeNumber(num), gradeName(std::move(name)), authFlag(auth) {}
};

} // namespace Client::Core::Events

namespace Client::Network::Handlers {

/**
 * @brief Zwarty handler domenowy SRP dla pakietow gildii (Guild).
 * Obsluguje czlonkow, uprawnienia, magazyn / skarbiec oraz wojny gildii.
 */
class GuildPacketDomainHandler {
public:
    GuildPacketDomainHandler() = default;
    explicit GuildPacketDomainHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager);
    ~GuildPacketDomainHandler() = default;

    void SetSocialManager(std::shared_ptr<Client::Gameplay::SocialManager> socialManager) noexcept {
        m_socialManager = std::move(socialManager);
    }

    [[nodiscard]] std::shared_ptr<Client::Gameplay::SocialManager> GetSocialManager() const noexcept {
        return m_socialManager;
    }

    // Glowny dyspozytor pakietu GC::GUILD
    static EterBase::PacketResult<void> HandleGuildPacket(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleRequestMakeGuild(std::span<const uint8_t> payload);

    // Podhandlery poszczegolnych subheaderow
    static EterBase::PacketResult<void> HandleSubLogin(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubLogout(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubRemove(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubList(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubGrade(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubGradeName(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubGradeAuth(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubInfo(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubComments(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubChangeExp(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubChangeMemberGrade(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubSkillInfo(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubChangeMemberGeneral(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubInvite(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubWar(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubWarPoint(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleSubMoneyChange(std::span<const uint8_t> payload);

    // Kodery pakietow wychodzacych
    static std::vector<uint8_t> EncodeAddMember(uint32_t vid);
    static std::vector<uint8_t> EncodeRemoveMember(uint32_t pid);
    static std::vector<uint8_t> EncodeChangeGradeName(uint8_t grade, std::string_view name);
    static std::vector<uint8_t> EncodeChangeGradeAuthority(uint8_t grade, uint8_t authority);
    static std::vector<uint8_t> EncodeOfferExp(uint32_t exp);
    static std::vector<uint8_t> EncodePostComment(std::string_view message);
    static std::vector<uint8_t> EncodeDeleteComment(uint32_t index);
    static std::vector<uint8_t> EncodeRefreshComments();
    static std::vector<uint8_t> EncodeChangeMemberGrade(uint32_t pid, uint8_t grade);
    static std::vector<uint8_t> EncodeUseSkill(uint32_t skillId, uint32_t targetVid);
    static std::vector<uint8_t> EncodeChangeMemberGeneral(uint32_t pid, uint8_t isGeneral);
    static std::vector<uint8_t> EncodeInviteAnswer(uint32_t guildId, uint8_t answer);
    static std::vector<uint8_t> EncodeDepositMoney(uint32_t amount);
    static std::vector<uint8_t> EncodeWithdrawMoney(uint32_t amount);
    static std::vector<uint8_t> EncodeAnswerMakeGuild(std::string_view guildName);

private:
    std::shared_ptr<Client::Gameplay::SocialManager> m_socialManager;
};

} // namespace Client::Network::Handlers
