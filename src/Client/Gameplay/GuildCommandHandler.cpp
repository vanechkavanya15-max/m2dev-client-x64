#include "StdAfx.h"
#include "GuildCommandHandler.h"
#include "../../UserInterface/Packet.h"
#include "../../EterBase/ModernLogger.h"
#include <vector>
#include <cstring>

namespace Client::Gameplay {

Core::Result<void, Core::CommandError> GuildCommandHandler::Handle(const GuildDepositExpCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort) {
    if (!networkPort || !networkPort->IsConnected()) {
        EterBase::ModernLogger::Error("GuildDepositExpCommand failed: Disconnected");
        return std::unexpected(Core::CommandError::Disconnected);
    }

    TPacketCGGuild packet{};
    packet.header = HEADER_CG_GUILD;
    packet.bySubHeader = GuildSub::CG::OFFER;
    packet.length = sizeof(TPacketCGGuild) + sizeof(uint32_t);

    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &cmd.exp, sizeof(uint32_t));

    auto res = networkPort->SendRaw(packet.header, payload);
    if (!res) {
        return std::unexpected(Core::CommandError::Disconnected);
    }
    return {};
}

Core::Result<void, Core::CommandError> GuildCommandHandler::Handle(const GuildDeclareWarCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort) {
    if (!networkPort || !networkPort->IsConnected()) {
        EterBase::ModernLogger::Error("GuildDeclareWarCommand failed: Disconnected");
        return std::unexpected(Core::CommandError::Disconnected);
    }

    constexpr uint8_t GUILD_SUB_CG_DECLARE_WAR = 15;
    
    TPacketCGGuild packet{};
    packet.header = HEADER_CG_GUILD;
    packet.bySubHeader = GUILD_SUB_CG_DECLARE_WAR;
    
    uint32_t targetId = cmd.targetGuildId.get();
    
    packet.length = sizeof(TPacketCGGuild) + sizeof(uint8_t) + sizeof(uint32_t);

    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &cmd.type, sizeof(uint8_t));
    std::memcpy(payload.data() + sizeof(packet) + sizeof(uint8_t), &targetId, sizeof(uint32_t));

    auto res = networkPort->SendRaw(packet.header, payload);
    if (!res) {
        return std::unexpected(Core::CommandError::Disconnected);
    }
    return {};
}

Core::Result<void, Core::CommandError> GuildCommandHandler::Handle(const GuildUseSkillCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort) {
    if (!networkPort || !networkPort->IsConnected()) {
        EterBase::ModernLogger::Error("GuildUseSkillCommand failed: Disconnected");
        return std::unexpected(Core::CommandError::Disconnected);
    }

    TPacketCGGuild packet{};
    packet.header = HEADER_CG_GUILD;
    packet.bySubHeader = GuildSub::CG::USE_SKILL;
    
    uint32_t targetVid = cmd.targetVid.get();
    
    packet.length = sizeof(TPacketCGGuild) + sizeof(uint32_t) + sizeof(uint32_t);

    std::vector<uint8_t> payload;
    payload.resize(packet.length);
    std::memcpy(payload.data(), &packet, sizeof(packet));
    std::memcpy(payload.data() + sizeof(packet), &cmd.skillId, sizeof(uint32_t));
    std::memcpy(payload.data() + sizeof(packet) + sizeof(uint32_t), &targetVid, sizeof(uint32_t));

    auto res = networkPort->SendRaw(packet.header, payload);
    if (!res) {
        return std::unexpected(Core::CommandError::Disconnected);
    }
    return {};
}

} // namespace Client::Gameplay
