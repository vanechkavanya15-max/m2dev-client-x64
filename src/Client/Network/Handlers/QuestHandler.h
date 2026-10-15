#pragma once

#include "../../../EterBase/Result.h"
#include <vector>
#include <cstdint>
#include <span>

namespace Client::Network::Handlers
{
    class QuestHandler
    {
    public:
        [[nodiscard]] EterBase::PacketResult<void> HandleQuestInfo(std::span<const uint8_t> buffer);
        [[nodiscard]] EterBase::PacketResult<void> HandleScript(std::span<const uint8_t> buffer);
        [[nodiscard]] EterBase::PacketResult<void> HandleWarp(std::span<const uint8_t> buffer);
    };
}
