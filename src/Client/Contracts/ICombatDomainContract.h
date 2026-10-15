#pragma once

#include <cstdint>

namespace Client::Contracts
{
    struct CombatVerdict
    {
        bool canAttack{false};
        float remainingCooldown{0.0f};
        float distanceToTarget{0.0f};
        uint8_t hitChancePercent{100};
        const char* reason{""};
    };

    /**
     * @brief Czysty interfejs domenowy walki dla Agentow AI i MCP.
     */
    class ICombatDomainContract
    {
    public:
        virtual ~ICombatDomainContract() = default;

        [[nodiscard]] virtual CombatVerdict EvaluateAttack(uint32_t targetVid) const = 0;
        [[nodiscard]] virtual float GetAttackCooldown() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetLastTargetVID() const noexcept = 0;

        virtual bool ExecuteAttack(uint32_t targetVid) = 0;
        virtual void UpdateCombatState(float deltaTime) = 0;
    };
}
