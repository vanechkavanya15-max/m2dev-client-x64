#pragma once

#include <cstdint>

namespace Domain
{

/**
 * @brief Represents a coordinate point for range checking purposes.
 */
struct Position
{
    float x;
    float y;
    float z;
};

/**
 * @brief A pure domain class responsible for verifying if a target is within the weapon's attack range.
 * 
 * This class adheres to the Single Responsibility Principle and is completely decoupled 
 * from any GUI or Python bindings. It computes the 2D distance squared using the X and Y 
 * axes to avoid expensive square root operations, optimizing performance.
 */
class BattleRangeChecker
{
public:
    /**
     * @brief Checks if a target is within the specified attack range.
     * 
     * Calculates the squared 2D distance between the attacker and the target
     * on the X-Y plane (ground plane in Metin2). Compares this squared distance against the squared weapon range.
     * 
     * @param attackerPosition The 3D position of the attacking entity.
     * @param targetPosition The 3D position of the target entity.
     * @param weaponRange The maximum attack range of the weapon.
     * @param targetRadius An optional target radius to account for large entities.
     * @return true if the target is within the weapon's attack range, false otherwise.
     */
    [[nodiscard]] static constexpr bool IsWithinRange(
        const Position& attackerPosition,
        const Position& targetPosition,
        float weaponRange,
        float targetRadius = 0.0f) noexcept
    {
        const float distanceX = targetPosition.x - attackerPosition.x;
        const float distanceY = targetPosition.y - attackerPosition.y;
        
        const float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
        
        const float effectiveRange = weaponRange + targetRadius;
        const float effectiveRangeSquared = effectiveRange * effectiveRange;
        
        return distanceSquared <= effectiveRangeSquared;
    }
};

} // namespace Domain
