#pragma once

#include "IPlayerStatsService.h"
#include <array>

namespace UserInterface::Services
{
    class PlayerStatsService final : public IPlayerStatsService
    {
    public:
        PlayerStatsService() = default;
        ~PlayerStatsService() override = default;

        void SetPoint(uint32_t type, int64_t value) override;
        int64_t GetPoint(uint32_t type) const override;
        const PlayerPointsView& GetPoints() const override;
        void Clear() override;

    private:
        std::array<int64_t, 255> m_points{};
        PlayerPointsView m_view{};
    };
}
