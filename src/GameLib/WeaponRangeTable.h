#pragma once

#include <cstdint>
#include "ItemData.h"

/**
 * @brief Provides a centralized table and getter for weapon combat ranges.
 * 
 * This class abstracts the default ranges of different weapon types to be used
 * during collision detection and attack range calculations.
 */
class WeaponRangeTable
{
public:
    // Weapon ranges constants
    static constexpr float WEAPON_RANGE_SWORD = 150.0f;
    static constexpr float WEAPON_RANGE_TWO_HANDED = 200.0f;
    static constexpr float WEAPON_RANGE_DAGGER = 100.0f;
    static constexpr float WEAPON_RANGE_BOW = 1000.0f;
    static constexpr float WEAPON_RANGE_BELL = 150.0f;
    static constexpr float WEAPON_RANGE_FAN = 150.0f;
    static constexpr float WEAPON_RANGE_DEFAULT = 100.0f;

    /**
     * @brief Gets the combat range for a specific weapon type.
     * 
     * @param weaponType The sub-type of the weapon (based on CItemData::EWeaponSubTypes).
     * @return The attack range in game units.
     */
    static constexpr float GetWeaponRange(uint8_t weaponType) noexcept
    {
        switch (weaponType)
        {
        case CItemData::WEAPON_SWORD:
            return WEAPON_RANGE_SWORD;
        case CItemData::WEAPON_TWO_HANDED:
            return WEAPON_RANGE_TWO_HANDED;
        case CItemData::WEAPON_DAGGER:
            return WEAPON_RANGE_DAGGER;
        case CItemData::WEAPON_BOW:
            return WEAPON_RANGE_BOW;
        case CItemData::WEAPON_BELL:
            return WEAPON_RANGE_BELL;
        case CItemData::WEAPON_FAN:
            return WEAPON_RANGE_FAN;
        default:
            return WEAPON_RANGE_DEFAULT;
        }
    }
};
