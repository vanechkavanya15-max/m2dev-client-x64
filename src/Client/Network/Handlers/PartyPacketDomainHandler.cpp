#include "PartyPacketDomainHandler.h"
#include "EterBase/LogModern.h"
#include <cstring>

namespace Client::Network::Handlers {

PartyPacketDomainHandler::PartyPacketDomainHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager)
    : m_socialManager(std::move(socialManager)) {
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyInvite(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyInvite)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyInvite za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyInvite));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyInvite packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyInvite));

    EterBase::ModernLogger::Info("PartyPacketDomainHandler: Otrzymano zaproszenie do grupy od lidera PID: {}", packet.leader_pid);

    Client::Core::Events::PartyInviteReceivedEvent event(packet.leader_pid);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyAdd(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyAdd)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyAdd za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyAdd));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyAdd packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyAdd));

    std::string memberName(packet.name, strnlen(packet.name, sizeof(packet.name)));
    EterBase::ModernLogger::Info("PartyPacketDomainHandler: Dodano czlonka grupy: {} (PID: {})", memberName, packet.pid);

    Client::Core::Events::PartyMemberAddedEvent event(packet.pid, memberName);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyUpdate(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyUpdate)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyUpdate za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyUpdate));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyUpdate packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyUpdate));

    std::vector<int16_t> affects;
    affects.reserve(PARTY_AFFECT_SLOT_MAX_NUM);
    for (int i = 0; i < PARTY_AFFECT_SLOT_MAX_NUM; ++i) {
        affects.push_back(packet.affects[i]);
    }

    EterBase::ModernLogger::Debug("PartyPacketDomainHandler: Aktualizacja czlonka PID: {}, HP: {}%, Stan: 0x{:02X}",
        packet.pid, packet.percent_hp, packet.state);

    Client::Core::Events::PartyMemberUpdatedEvent event(packet.pid, packet.state, packet.percent_hp, std::move(affects));
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyRemove(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyRemove)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyRemove za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyRemove));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyRemove packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyRemove));

    EterBase::ModernLogger::Info("PartyPacketDomainHandler: Usuniecie czlonka PID: {} z grupy", packet.pid);

    Client::Core::Events::PartyMemberRemovedEvent event(packet.pid);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyLink(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyLink)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyLink za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyLink));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyLink packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyLink));

    EterBase::ModernLogger::Debug("PartyPacketDomainHandler: Powiazanie gracza PID: {} z VID: {}", packet.pid, packet.vid);

    Client::Core::Events::PartyMemberLinkedEvent event(packet.pid, packet.vid);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyUnlink(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyUnlink)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyUnlink za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyUnlink));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyUnlink packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyUnlink));

    EterBase::ModernLogger::Debug("PartyPacketDomainHandler: Rozlaczenie gracza PID: {} z VID: {}", packet.pid, packet.vid);

    Client::Core::Events::PartyMemberUnlinkedEvent event(packet.pid, packet.vid);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> PartyPacketDomainHandler::HandlePartyParameter(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPartyParameter)) {
        EterBase::ModernLogger::Error("PartyPacketDomainHandler: Bufor TPacketGCPartyParameter za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCPartyParameter));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    TPacketGCPartyParameter packet{};
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCPartyParameter));

    EterBase::ModernLogger::Info("PartyPacketDomainHandler: Zmiana trybu dystrybucji grupy: {}", packet.bDistributeMode);

    Client::Core::Events::PartyParameterChangedEvent event(packet.bDistributeMode);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartyInvite(uint32_t vid) {
    TPacketCGPartyInvite packet{};
    packet.header = CG::PARTY_INVITE;
    packet.length = sizeof(TPacketCGPartyInvite);
    packet.vid = vid;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartyInviteAnswer(uint32_t leaderVid, uint8_t accept) {
    TPacketCGPartyInviteAnswer packet{};
    packet.header = CG::PARTY_INVITE_ANSWER;
    packet.length = sizeof(TPacketCGPartyInviteAnswer);
    packet.leader_pid = leaderVid;
    packet.accept = accept;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartyRemove(uint32_t pid) {
    TPacketCGPartyRemove packet{};
    packet.header = CG::PARTY_REMOVE;
    packet.length = sizeof(TPacketCGPartyRemove);
    packet.pid = pid;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartySetState(uint32_t vid, uint8_t state, uint8_t flag) {
    TPacketCGPartySetState packet{};
    packet.header = CG::PARTY_SET_STATE;
    packet.length = sizeof(TPacketCGPartySetState);
    packet.dwVID = vid;
    packet.byState = state;
    packet.byFlag = flag;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartyUseSkill(uint8_t skillIndex, uint32_t vid) {
    TPacketCGPartyUseSkill packet{};
    packet.header = CG::PARTY_USE_SKILL;
    packet.length = sizeof(TPacketCGPartyUseSkill);
    packet.bySkillIndex = skillIndex;
    packet.dwTargetVID = vid;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> PartyPacketDomainHandler::EncodePartyParameter(uint8_t distributeMode) {
    TPacketCGPartyParameter packet{};
    packet.header = CG::PARTY_PARAMETER;
    packet.length = sizeof(TPacketCGPartyParameter);
    packet.bDistributeMode = distributeMode;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network::Handlers
