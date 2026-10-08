#pragma once

#include <memory>
#include <string_view>
#include <format>
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "SocialDomain.h"

namespace Client::Gameplay {

    enum class CommandError : uint8_t {
        None = 0,
        NotInParty,
        AlreadyInParty,
        NotLeader,
        PlayerNotFound,
        InvalidTarget
    };

    [[nodiscard]] constexpr std::string_view ToString(CommandError err) noexcept {
        switch (err) {
            case CommandError::None: return "None";
            case CommandError::NotInParty: return "NotInParty";
            case CommandError::AlreadyInParty: return "AlreadyInParty";
            case CommandError::NotLeader: return "NotLeader";
            case CommandError::PlayerNotFound: return "PlayerNotFound";
            case CommandError::InvalidTarget: return "InvalidTarget";
        }
        return "UnknownCommandError";
    }

    class PartyCommandHandler {
    public:
        explicit PartyCommandHandler(std::shared_ptr<SocialManager> socialManager);

        EterBase::Result<void, CommandError> Invite(EterBase::EntityId playerVid, EterBase::EntityId targetVid);
        EterBase::Result<void, CommandError> Leave(EterBase::EntityId playerVid);
        EterBase::Result<void, CommandError> ChangeLeader(EterBase::EntityId playerVid, EterBase::EntityId newLeaderVid);

    private:
        std::shared_ptr<SocialManager> m_socialManager;
    };

} // namespace Client::Gameplay

template <>
struct std::formatter<Client::Gameplay::CommandError> : std::formatter<std::string_view> {
    auto format(Client::Gameplay::CommandError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Gameplay::ToString(err), ctx);
    }
};
