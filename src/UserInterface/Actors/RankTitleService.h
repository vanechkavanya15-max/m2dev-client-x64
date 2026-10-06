#pragma once
#include <unordered_map>
#include <string>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::Actors {
    class RankTitleService {
    public:
        EterBase::PacketResult<void> Clear() noexcept;
    private:
        std::unordered_map<EterBase::EntityId, uint32_t> m_ranks;
        std::unordered_map<EterBase::EntityId, EterBase::GuildId> m_guilds;
    };
}
