#include "StdAfx.h"
#include "StranglerFacade.h"
#include <iostream>

namespace Client::Bridge {

StranglerFacade& StranglerFacade::Instance() noexcept {
    static StranglerFacade s_instance;
    return s_instance;
}

void StranglerFacade::Initialize(CNetworkStream* networkStream) {
    m_port = std::make_shared<NetworkStreamPort>(networkStream);
    m_session = std::make_shared<Client::Core::GameSession>(m_port);
}

void StranglerFacade::Shutdown() {
    m_session.reset();
    m_port.reset();
}

Client::Core::WorldContext& StranglerFacade::GetWorldContext() noexcept {
    static Client::Core::WorldContext s_dummyContext;
    if (m_session) {
        return m_session->GetWorldContext();
    }
    return s_dummyContext;
}

bool StranglerFacade::ExecuteAttack(uint32_t victimVid, uint8_t attackType) {
    if (!m_session) return false;
    Client::Core::AttackCommand cmd{
        .targetVid = Client::Core::EntityVid(victimVid),
        .attackType = attackType
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteMove(float x, float y, float z, float rot, uint8_t moveType) {
    if (!m_session) return false;
    Client::Core::MoveCommand cmd{
        .destination = Client::Core::MapCoords{x, y, z},
        .rotation = rot,
        .moveType = moveType
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteUseSkill(uint32_t skillId, uint32_t targetVid) {
    if (!m_session) return false;
    Client::Core::UseSkillCommand cmd{
        .skillId = Client::Core::SkillId(skillId),
        .targetVid = Client::Core::EntityVid(targetVid)
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteUseItem(uint16_t slot) {
    if (!m_session) return false;
    Client::Core::UseItemCommand cmd{
        .slot = Client::Core::ItemSlot(slot)
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteDropItem(uint16_t slot, uint32_t count) {
    if (!m_session) return false;
    Client::Core::DropItemCommand cmd{
        .slot = Client::Core::ItemSlot(slot),
        .count = count
    };
    // DropItem Command Execute w sesji
    return true;
}

bool StranglerFacade::ExecutePickupItem(uint32_t itemVid) {
    if (!m_session) return false;
    Client::Core::PickupCommand cmd{
        .itemVid = Client::Core::EntityVid(itemVid)
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteChat(std::string_view msg, uint8_t type) {
    if (!m_session) return false;
    Client::Core::ChatCommand cmd{
        .message = std::string(msg),
        .chatType = type
    };
    auto res = m_session->Execute(cmd);
    return res.has_value();
}

bool StranglerFacade::ExecuteWhisper(std::string_view target, std::string_view msg) {
    if (!m_session) return false;
    Client::Core::WhisperCommand cmd{
        .recipientName = std::string(target),
        .message = std::string(msg)
    };
    // Whisper komenda
    return true;
}

} // namespace Client::Bridge
