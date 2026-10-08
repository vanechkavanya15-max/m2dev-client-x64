#include "StdAfx.h"
#include "SendUseSkillPacket.h"
#include "../../../EterLib/NetStream.h"
#include "../../../EterBase/LogModern.h"
#include <cstring> // For std::memset

namespace UserInterface::Network::Senders {

#pragma pack(push, 1)
/**
 * @brief Lokalna struktura proxy pakietu uzycia umiejetnosci.
 * 
 * Izoluje aplikacje od starych definicji (TPacketCGUseSkill), dbajac o poprawne
 * 1-bajtowe wyrownanie pamieci i brak notacji wegierskiej.
 */
struct ProxyPacketCGUseSkill
{
    uint16_t header;       ///< Identyfikator naglowka pakietu sieciowego.
    uint16_t length;       ///< Dlugosc struktury pakietu.
    uint32_t skillId;      ///< Identyfikator VNUM umiejetnosci.
    uint32_t targetId;     ///< Identyfikator wirtualny (VID) celu umiejetnosci.
};
static_assert(sizeof(ProxyPacketCGUseSkill) == 12, "ProxyPacketCGUseSkill musi miec dokladnie 12 bajtow (wyrownanie 1-bajtowe)");
#pragma pack(pop)

EterBase::PacketDispatchResult<void> SendUseSkillHandler::SendUseSkill(
    EterBase::SkillId skillId, 
    std::optional<EterBase::EntityId> targetId, 
    CNetworkStream* networkStream)
{
    if (!networkStream)
    {
        EterBase::ModernLogger::Error("SendUseSkill failed: NetworkStream is null (skill: {}, target: {})", 
                                      skillId.value(), 
                                      targetId.transform([](auto id) { return id.value(); }).value_or(0));
        return EterBase::MakeError(EterBase::PacketDispatchError::Disconnected);
    }

    ProxyPacketCGUseSkill packet{};
    // Uzywamy sztywnej wartosci 0x0402 bazujac na CG::USE_SKILL w src/UserInterface/Packet.h
    packet.header = 0x0402;
    packet.length = static_cast<uint16_t>(sizeof(packet));
    packet.skillId = skillId.value();
    packet.targetId = targetId.transform([](auto id) { return id.value(); }).value_or(0);

    std::span<const uint8_t> buffer(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

    if (!networkStream->Send(static_cast<int>(buffer.size()), buffer.data()))
    {
        EterBase::ModernLogger::Error("SendUseSkill failed to send buffer to network stream (skill: {}, target: {})", 
                                      skillId.value(), packet.targetId);
        return EterBase::MakeError(EterBase::PacketDispatchError::QueueFull);
    }

    EterBase::ModernLogger::Debug("SendUseSkill succeeded (skill: {}, target: {})", 
                                  skillId.value(), packet.targetId);

    // Odpiecie od GUI (Event-driven architecture)
    UserInterface::Core::EventBus::GetInstance().Publish(SkillUseRequestedEvent{skillId, targetId});

    return {};
}

} // namespace UserInterface::Network::Senders
