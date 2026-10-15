#pragma once

#include <cstdint>

namespace Client::Gameplay
{
    /**
     * @brief Stan punktow i atrybutow postaci gracza.
     */
    struct PlayerPointsView
    {
        uint32_t hp{0};
        uint32_t maxHp{0};
        uint32_t sp{0};
        uint32_t maxSp{0};
        uint32_t stamina{0};
        uint32_t maxStamina{0};
        uint64_t exp{0};
        uint64_t nextExp{0};
        int64_t gold{0};
        uint8_t level{1};
        uint16_t statPoints{0};
        uint16_t skillPoints{0};
    };

    /**
     * @brief Interfejs mikro-serwisu kalkulacji i przechowywania statystyk gracza.
     */
    class IPlayerStatsService
    {
    public:
        virtual ~IPlayerStatsService() = default;

        virtual void SetPoint(uint32_t type, int64_t value) = 0;
        virtual int64_t GetPoint(uint32_t type) const = 0;
        virtual const PlayerPointsView& GetPoints() const = 0;
        virtual void Clear() = 0;
    };
}
