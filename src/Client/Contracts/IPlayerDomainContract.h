#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Client::Contracts
{
    struct PlayerCoordinates
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float rotation{0.0f};
    };

    struct PlayerStatusSnapshot
    {
        uint32_t vid{0};
        std::string name;
        int level{1};
        int64_t hp{0};
        int64_t maxHp{0};
        int64_t sp{0};
        int64_t maxSp{0};
        int64_t yang{0};
        int64_t exp{0};
        PlayerCoordinates coords;
        uint32_t targetVid{0};
        bool isCombatActive{false};
        std::string mapName;
    };

    /**
     * @brief Czysty interfejs domenowy gracza dla Agentow AI i Mostka MCP.
     * Pozbawiony zaleznosci od Windows API i Direct3D 9.
     */
    class IPlayerDomainContract
    {
    public:
        virtual ~IPlayerDomainContract() = default;

        [[nodiscard]] virtual uint32_t GetPlayerVID() const noexcept = 0;
        [[nodiscard]] virtual std::string_view GetPlayerName() const noexcept = 0;
        [[nodiscard]] virtual int GetPlayerLevel() const noexcept = 0;

        [[nodiscard]] virtual int64_t GetCurrentHP() const noexcept = 0;
        [[nodiscard]] virtual int64_t GetMaxHP() const noexcept = 0;
        [[nodiscard]] virtual int64_t GetCurrentSP() const noexcept = 0;
        [[nodiscard]] virtual int64_t GetMaxSP() const noexcept = 0;
        [[nodiscard]] virtual int64_t GetYang() const noexcept = 0;
        [[nodiscard]] virtual int64_t GetExp() const noexcept = 0;

        [[nodiscard]] virtual PlayerCoordinates GetCoordinates() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetTargetVID() const noexcept = 0;
        [[nodiscard]] virtual bool IsInCombat() const noexcept = 0;

        [[nodiscard]] virtual PlayerStatusSnapshot CaptureSnapshot() const = 0;

        [[nodiscard]] virtual int64_t GetStatus(int statusType) const
        {
            switch (statusType)
            {
                case 1:  return GetPlayerLevel();
                case 3:  return GetExp();
                case 5:  return GetCurrentHP();
                case 6:  return GetMaxHP();
                case 7:  return GetCurrentSP();
                case 8:  return GetMaxSP();
                case 11: return GetYang();
                default: return 0;
            }
        }

        virtual bool RequestMove(float /*x*/, float /*y*/) { return true; }
    };
}
