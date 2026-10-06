#pragma once

#include <cstdint>
#include <algorithm>

/**
 * @brief Namespace containing combat math utilities for health calculations.
 */
namespace HealthPercentCalc
{
    /**
     * @brief Calculates the health percentage safely, protecting against division by zero.
     * 
     * This function evaluates the health percentage of an actor based on current and max health.
     * It ensures the result is strictly between 0 and 100, avoiding overflow or underflow, 
     * and returns 0 if the max health is 0.
     * 
     * @param currentHealth The current health points of the actor.
     * @param maxHealth The maximum health points of the actor.
     * @return uint8_t The percentage of health remaining, from 0 to 100.
     */
    inline uint8_t Calculate(uint32_t currentHealth, uint32_t maxHealth)
    {
        if (maxHealth == 0)
        {
            return 0;
        }

        if (currentHealth >= maxHealth)
        {
            return 100;
        }

        // Use uint64_t to prevent overflow during intermediate multiplication
        uint32_t percentage = static_cast<uint32_t>((static_cast<uint64_t>(currentHealth) * 100) / maxHealth);

        return static_cast<uint8_t>(std::clamp<uint32_t>(percentage, 0, 100));
    }
}
