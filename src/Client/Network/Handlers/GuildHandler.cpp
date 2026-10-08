#include "StdAfx.h"
#include "GuildHandler.h"
#include "../../../UserInterface/Packet.h"
#include "../../../UserInterface/Core/EventBus.h"
#include <cstring>

namespace Client::Network::Handlers {

GuildHandler::GuildHandler(Client::Gameplay::SocialManager& socialManager)
    : m_socialManager(socialManager) {}

EterBase::PacketResult<void> GuildHandler::HandleGuildPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCGuild)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* header = reinterpret_cast<const TPacketGCGuild*>(payload.data());
    size_t offset = sizeof(TPacketGCGuild);
    size_t remainingSize = payload.size() - offset;

    switch (header->subheader) {
        case GuildSub::GC::LIST: {
            while (remainingSize > 0) {
                if (remainingSize < sizeof(TPacketGCGuildSubMember)) {
                    return std::unexpected(EterBase::PacketError::BufferUnderflow);
                }
                const auto* memberPack = reinterpret_cast<const TPacketGCGuildSubMember*>(payload.data() + offset);
                offset += sizeof(TPacketGCGuildSubMember);
                remainingSize -= sizeof(TPacketGCGuildSubMember);

                std::string memberName = "";
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
                    // Update permissions based on grade/auth flag if applicable, here we set general permissions for demo
                    uint32_t permissions = memberPack->byGrade;
                    Client::Gameplay::GuildMember newMember(EterBase::EntityId{memberPack->pid}, memberName, permissions);
                    
                    if (guild->GetMember(EterBase::EntityId{memberPack->pid})) {
                         guild->RemoveMember(EterBase::EntityId{memberPack->pid}); // Refresh
                    }
                    guild->AddMember(newMember);
                }
            }
            break;
        }
        case GuildSub::GC::CHANGE_EXP: {
            if (remainingSize < sizeof(uint8_t) + sizeof(uint32_t)) {
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }
            uint8_t level;
            uint32_t exp;
            std::memcpy(&level, payload.data() + offset, sizeof(uint8_t));
            offset += sizeof(uint8_t);
            std::memcpy(&exp, payload.data() + offset, sizeof(uint32_t));
            
            auto guild = m_socialManager.GetGuild();
            if (guild) {
                guild->SetExp(level, exp);
            }
            
            UserInterface::Core::EventBus::GetInstance().Publish(GuildExpUpdatedEvent(level, exp));
            break;
        }
        case GuildSub::GC::MONEY_CHANGE: {
            if (remainingSize < sizeof(uint32_t)) {
                return std::unexpected(EterBase::PacketError::BufferUnderflow);
            }
            uint32_t money;
            std::memcpy(&money, payload.data() + offset, sizeof(uint32_t));
            
            auto guild = m_socialManager.GetGuild();
            if (guild) {
                guild->SetBank(money);
            }
            
            UserInterface::Core::EventBus::GetInstance().Publish(GuildBankUpdatedEvent(money));
            break;
        }
        // Additional subheaders could be processed here
        default:
            break;
    }
    
    return {};
}

EterBase::PacketResult<void> GuildHandler::HandleMarkUpdatePacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCMarkUpdate)) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCMarkUpdate*>(payload.data());
    
    auto guild = m_socialManager.GetGuild();
    if (guild && guild->GetId().value() == packet->guildID) {
        guild->SetMark(std::to_string(packet->imgIdx));
    }
    
    UserInterface::Core::EventBus::GetInstance().Publish(GuildMarkUpdatedEvent(packet->guildID, packet->imgIdx));
    
    return {};
}

} // namespace Client::Network::Handlers
