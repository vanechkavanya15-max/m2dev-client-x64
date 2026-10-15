#include "BeaviumCombatPolicy.h"
#include <algorithm>

namespace Client::Gameplay
{
    uint32_t BeaviumCombatPolicy::CalculateObfuscatedTargetKey(uint32_t dwTargetVID) const noexcept
    {
        return dwTargetVID ^ COMBAT_OBFUSCATION_KEY;
    }

    uint32_t BeaviumCombatPolicy::CalculateHitChance(uint32_t dwLevel, uint32_t dwStat) const noexcept
    {
        // Specyfikacja z inzynierii wstecznej Beavium (BEAVIUM_ATTACK_CALCULATION_SPECIFICATION.md):
        // HitChance = 70% + (Level + 2 * Stat) / 9, z ograniczeniem do przedzialu [70%, 100%]
        const uint32_t calculated = 70 + (dwLevel + 2 * dwStat) / 9;
        return std::clamp(calculated, 70u, 100u);
    }

    bool BeaviumCombatPolicy::CanAttackInstance(uint32_t dwTargetVID)
    {
        if (dwTargetVID == 0)
        {
            return false;
        }

        // Sprawdzenie cooldownu miedzy ciosami (ochrona przed desynchronizacja z serwerem Beavium)
        if (m_fLastAttackTime < m_fAttackCooldown)
        {
            return false;
        }

        return true;
    }

    bool BeaviumCombatPolicy::AttackTarget(uint32_t dwTargetVID)
    {
        if (!CanAttackInstance(dwTargetVID))
        {
            return false;
        }

        m_dwLastTargetVID = dwTargetVID;
        m_fLastAttackTime = 0.0f;
        return true;
    }

    void BeaviumCombatPolicy::UpdateAttackState(float fElapsedTime)
    {
        m_fLastAttackTime += fElapsedTime;
    }
}
