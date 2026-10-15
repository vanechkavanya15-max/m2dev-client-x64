#pragma once

#include "UserInterface/Contracts/ICombatPolicy.h"
#include <cstdint>

namespace Client::Gameplay
{
    /**
     * @brief Dedykowana polityka walki dla serwera Beavium.
     * Implementuje interfejs ICombatPolicy.
     * 
     * SPECYFIKACJA Z REVERSE ENGINEERINGU:
     * 1. Klucz XOR ataku: COMBAT_OBFUSCATION_KEY = 0xE73AC1DA
     * 2. Formula szansy trafienia (Hit Chance): 70% + (Level + 2 * Stat) / 9,
     *    z obcieciem do scislego przedzialu [70%, 100%].
     * 3. Walidacja celowania i zarzadzanie stanem walki gracza bezposrednio w profilu.
     */
    class BeaviumCombatPolicy : public ICombatPolicy
    {
    public:
        static constexpr uint32_t COMBAT_OBFUSCATION_KEY = 0xE73AC1DA;

        BeaviumCombatPolicy() = default;
        ~BeaviumCombatPolicy() override = default;

        [[nodiscard]] bool CanAttackInstance(uint32_t dwTargetVID) override;
        [[nodiscard]] bool AttackTarget(uint32_t dwTargetVID) override;
        void UpdateAttackState(float fElapsedTime) override;

        [[nodiscard]] uint32_t CalculateObfuscatedTargetKey(uint32_t dwTargetVID) const noexcept;
        [[nodiscard]] uint32_t CalculateHitChance(uint32_t dwLevel, uint32_t dwStat) const noexcept;

        void SetAttackCooldown(float fCooldown) noexcept { m_fAttackCooldown = fCooldown; }
        [[nodiscard]] float GetAttackCooldown() const noexcept { return m_fAttackCooldown; }
        [[nodiscard]] uint32_t GetLastTargetVID() const noexcept { return m_dwLastTargetVID; }

    private:
        float m_fLastAttackTime{0.25f}; // Domyslnie gotowy do ataku
        float m_fAttackCooldown{0.25f}; // Bezpieczny interwal ataku (250 ms)
        uint32_t m_dwLastTargetVID{0};
    };
}
