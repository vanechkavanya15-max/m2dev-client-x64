#pragma once

#include <cstdint>
#include <memory>
#include "../Core/Result.h"
#include "../Core/StrongTypes.h"
#include "../Core/DomainCommands.h"
#include "../Core/INetworkPort.h"

namespace Client::Gameplay {

struct GuildDepositExpCommand {
    uint32_t exp;
};

struct GuildDeclareWarCommand {
    uint8_t type;
    Core::GuildId targetGuildId;
};

struct GuildUseSkillCommand {
    uint32_t skillId;
    Core::EntityVid targetVid;
};

class GuildCommandHandler {
public:
    static Core::Result<void, Core::CommandError> Handle(const GuildDepositExpCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort);
    static Core::Result<void, Core::CommandError> Handle(const GuildDeclareWarCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort);
    static Core::Result<void, Core::CommandError> Handle(const GuildUseSkillCommand& cmd, std::shared_ptr<Core::INetworkPort> networkPort);
};

} // namespace Client::Gameplay
