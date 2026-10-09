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
 * @brief Zdarzenie domenowe otrzymania zaproszenia do grupy od lidera.
 */
struct PartyInviteReceivedEvent : public Client::Core::IEvent {
    uint32_t leaderPid{0};

    constexpr PartyInviteReceivedEvent() noexcept = default;
    constexpr explicit PartyInviteReceivedEvent(uint32_t pid) noexcept : leaderPid(pid) {}
};

/**
 * @brief Zdarzenie domenowe dolaczenia nowego czlonka do grupy.
 */
struct PartyMemberAddedEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    std::string name;

    PartyMemberAddedEvent(uint32_t p, std::string n) : pid(p), name(std::move(n)) {}
};

/**
 * @brief Zdarzenie domenowe aktualizacji stanu/HP/affectow czlonka grupy.
 */
struct PartyMemberUpdatedEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    uint8_t state{0};
    uint8_t percentHp{0};
    std::vector<int16_t> affects;

    PartyMemberUpdatedEvent(uint32_t p, uint8_t s, uint8_t hp, std::vector<int16_t> aff)
        : pid(p), state(s), percentHp(hp), affects(std::move(aff)) {}
};

/**
 * @brief Zdarzenie domenowe usuniecia czlonka z grupy.
 */
struct PartyMemberRemovedEvent : public Client::Core::IEvent {
    uint32_t pid{0};

    constexpr explicit PartyMemberRemovedEvent(uint32_t p) noexcept : pid(p) {}
};

/**
 * @brief Zdarzenie domenowe powiazania czlonka grupy z VID instancji w swiecie.
 */
struct PartyMemberLinkedEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    uint32_t vid{0};

    constexpr PartyMemberLinkedEvent(uint32_t p, uint32_t v) noexcept : pid(p), vid(v) {}
};

/**
 * @brief Zdarzenie domenowe rozlaczenia czlonka grupy z VID w swiecie.
 */
struct PartyMemberUnlinkedEvent : public Client::Core::IEvent {
    uint32_t pid{0};
    uint32_t vid{0};

    constexpr PartyMemberUnlinkedEvent(uint32_t p, uint32_t v) noexcept : pid(p), vid(v) {}
};

/**
 * @brief Zdarzenie domenowe zmiany konfiguracji podzialu lootu/doswiadczenia grupy.
 */
struct PartyParameterChangedEvent : public Client::Core::IEvent {
    uint8_t distributeMode{0};

    constexpr explicit PartyParameterChangedEvent(uint8_t mode) noexcept : distributeMode(mode) {}
};

} // namespace Client::Core::Events

namespace Client::Network::Handlers {

/**
 * @brief Zwarty handler domenowy SRP dla pakietow grupy (Party).
 * Obsluguje pelen cykl zycia druzyny: zaproszenia, dodawanie, usuwanie, HP,
 * synchronizacje VID oraz tryby podzialu lootu i doswiadczenia.
 */
class PartyPacketDomainHandler {
public:
    PartyPacketDomainHandler() = default;
    explicit PartyPacketDomainHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager);
    ~PartyPacketDomainHandler() = default;

    void SetSocialManager(std::shared_ptr<Client::Gameplay::SocialManager> socialManager) noexcept {
        m_socialManager = std::move(socialManager);
    }

    [[nodiscard]] std::shared_ptr<Client::Gameplay::SocialManager> GetSocialManager() const noexcept {
        return m_socialManager;
    }

    // Handlery wejsciowe pakietow (Inbound decoders)
    static EterBase::PacketResult<void> HandlePartyInvite(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyAdd(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyUpdate(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyRemove(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyLink(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyUnlink(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandlePartyParameter(std::span<const uint8_t> payload);

    // Koder pakietow wychodzacych (Outbound encoders)
    static std::vector<uint8_t> EncodePartyInvite(uint32_t vid);
    static std::vector<uint8_t> EncodePartyInviteAnswer(uint32_t leaderVid, uint8_t accept);
    static std::vector<uint8_t> EncodePartyRemove(uint32_t pid);
    static std::vector<uint8_t> EncodePartySetState(uint32_t vid, uint8_t state, uint8_t flag);
    static std::vector<uint8_t> EncodePartyUseSkill(uint8_t skillIndex, uint32_t vid);
    static std::vector<uint8_t> EncodePartyParameter(uint8_t distributeMode);

private:
    std::shared_ptr<Client::Gameplay::SocialManager> m_socialManager;
};

} // namespace Client::Network::Handlers
