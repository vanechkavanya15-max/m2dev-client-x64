#pragma once

#include <cstdint>
#include <expected>
#include <array>
#include <string_view>

namespace Client::Gameplay
{
    /**
     * @brief Typy automatycznych mikstur (autopoty).
     */
    enum class AutoPotionType : uint8_t
    {
        Health = 0,
        Mana = 1,
        Count
    };

    /**
     * @brief Kody bledow dla menedzera autopotow.
     */
    enum class AutoPotionError : uint8_t
    {
        None = 0,
        InvalidThreshold,
        InvalidAmount,
        ItemEmpty,
        CooldownActive,
        AlreadyActive,
        NotActive
    };

    template <typename T, typename E>
    using Result = std::expected<T, E>;

    /**
     * @brief Struktura przechowujaca stan pojedynczego typu autopota.
     */
    struct AutoPotionState
    {
        bool isActive{false};
        float thresholdPercentage{0.0f}; // Od 0.0 do 100.0
        uint32_t remainingAmount{0};     // Pozostala ilosc punktow do przywrocenia (z puli mikstury)
        uint64_t lastUseTime{0};         // Czas ostatniego uzycia (do cooldownu)
    };

    /**
     * @brief Klasa odpowiedzialna za zarzadzanie automatycznym piciem mikstur (Autopoty).
     * Zgodna z architektura ZERO-CONFLICT i calkowicie niezalezna.
     */
    class AutoPotionManager
    {
    public:
        constexpr AutoPotionManager() noexcept = default;
        ~AutoPotionManager() = default;

        AutoPotionManager(const AutoPotionManager&) = delete;
        AutoPotionManager& operator=(const AutoPotionManager&) = delete;
        AutoPotionManager(AutoPotionManager&&) = delete;
        AutoPotionManager& operator=(AutoPotionManager&&) = delete;

        /**
         * @brief Aktywuje automatyczne leczenie dla danego typu mikstury.
         */
        constexpr Result<void, AutoPotionError> Enable(AutoPotionType type, float thresholdPct, uint32_t amount) noexcept
        {
            if (thresholdPct <= 0.0f || thresholdPct > 100.0f)
            {
                return std::unexpected(AutoPotionError::InvalidThreshold);
            }

            if (amount == 0)
            {
                return std::unexpected(AutoPotionError::InvalidAmount);
            }

            if (type >= AutoPotionType::Count) return std::unexpected(AutoPotionError::NotActive);
            auto& state = m_states[static_cast<size_t>(type)];
            if (state.isActive)
            {
                return std::unexpected(AutoPotionError::AlreadyActive);
            }

            state.isActive = true;
            state.thresholdPercentage = thresholdPct;
            state.remainingAmount = amount;
            state.lastUseTime = 0;
            
            return {};
        }

        /**
         * @brief Dezaktywuje automatyczne leczenie.
         */
        constexpr Result<void, AutoPotionError> Disable(AutoPotionType type) noexcept
        {
            if (type >= AutoPotionType::Count) return std::unexpected(AutoPotionError::NotActive);
            auto& state = m_states[static_cast<size_t>(type)];
            if (!state.isActive)
            {
                return std::unexpected(AutoPotionError::NotActive);
            }

            state.isActive = false;
            return {};
        }

        /**
         * @brief Zwraca true, jesli dany typ autopota jest aktywny.
         */
        [[nodiscard]] constexpr bool IsActive(AutoPotionType type) const noexcept
        {
            if (type >= AutoPotionType::Count) return false;
            return m_states[static_cast<size_t>(type)].isActive;
        }

        /**
         * @brief Pobiera aktualny prog uzycia w procentach (0.0 - 100.0).
         */
        [[nodiscard]] constexpr float GetThreshold(AutoPotionType type) const noexcept
        {
            if (type >= AutoPotionType::Count) return 0.0f;
            return m_states[static_cast<size_t>(type)].thresholdPercentage;
        }

        /**
         * @brief Pobiera pozostala ilosc mikstury.
         */
        [[nodiscard]] constexpr uint32_t GetRemainingAmount(AutoPotionType type) const noexcept
        {
            if (type >= AutoPotionType::Count) return 0;
            return m_states[static_cast<size_t>(type)].remainingAmount;
        }

        /**
         * @brief Zwieksza pozostala ilosc w aktywnej miksturze (np. przy podniesieniu kolejnej butelki).
         */
        constexpr Result<void, AutoPotionError> AddAmount(AutoPotionType type, uint32_t amountToAdd) noexcept
        {
            if (type >= AutoPotionType::Count) return std::unexpected(AutoPotionError::NotActive);
            auto& state = m_states[static_cast<size_t>(type)];
            if (!state.isActive)
            {
                return std::unexpected(AutoPotionError::NotActive);
            }

            if (UINT32_MAX - state.remainingAmount < amountToAdd)
            {
                state.remainingAmount = UINT32_MAX;
            }
            else
            {
                state.remainingAmount += amountToAdd;
            }

            return {};
        }

        /**
         * @brief Przetwarza logike autopota. Sprawdza czy HP/MP spadlo ponizej progu i leczy.
         * @return Zwraca ilosc przywroconych punktow lub blad jesli mikstura sie skonczyla (lub trwa cooldown).
         */
        constexpr Result<uint32_t, AutoPotionError> Process(AutoPotionType type, uint32_t currentPoints, uint32_t maxPoints, uint64_t currentTime, uint64_t cooldownMs = 1000) noexcept
        {
            if (type >= AutoPotionType::Count) return std::unexpected(AutoPotionError::NotActive);
            auto& state = m_states[static_cast<size_t>(type)];
            if (!state.isActive)
            {
                return std::unexpected(AutoPotionError::NotActive);
            }

            if (maxPoints == 0)
            {
                return 0; // Zabezpieczenie na wypadek braku max punktow
            }

            if (state.remainingAmount == 0)
            {
                state.isActive = false; // Koniec mikstury
                return std::unexpected(AutoPotionError::ItemEmpty);
            }

            if (state.lastUseTime != 0 && currentTime < state.lastUseTime + cooldownMs)
            {
                return std::unexpected(AutoPotionError::CooldownActive);
            }

            const float currentPct = (static_cast<float>(currentPoints) / static_cast<float>(maxPoints)) * 100.0f;

            if (currentPct <= state.thresholdPercentage)
            {
                const uint32_t missingPoints = maxPoints - currentPoints;
                if (missingPoints == 0)
                {
                    return 0;
                }

                const uint32_t healAmount = (missingPoints > state.remainingAmount) ? state.remainingAmount : missingPoints;
                state.remainingAmount -= healAmount;
                state.lastUseTime = currentTime;

                if (state.remainingAmount == 0)
                {
                    state.isActive = false;
                }

                return healAmount;
            }

            return 0;
        }

    private:
        std::array<AutoPotionState, static_cast<size_t>(AutoPotionType::Count)> m_states{};
    };
}
