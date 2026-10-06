#pragma once

#include <cstdint>
#include <span>
#include <optional>

/**
 * @brief Represents the types of potions available for auto-selection.
 */
enum class PotionType : uint8_t
{
    Red,    ///< HP recovering potion.
    Blue    ///< SP recovering potion.
};

/**
 * @brief Handles automatic potion selection logic independent of the GUI.
 */
class PotionSelector
{
public:
    /**
     * @brief Searches an inventory span to find the first suitable potion of the requested type.
     *
     * Scans the provided inventory span for a potion that matches the given PotionType.
     * 
     * @param inventory A span of item VNUMs representing the inventory contents.
     * @param type The desired potion type to find.
     * @return std::optional<uint32_t> The zero-based slot index of the potion if found, otherwise std::nullopt.
     */
    static std::optional<uint32_t> FindPotion(std::span<const uint32_t> inventory, PotionType type)
    {
        for (uint32_t index = 0; index < inventory.size(); ++index)
        {
            uint32_t vnum = inventory[index];
            if (vnum == 0)
                continue;

            if (IsPotionMatch(vnum, type))
            {
                return index;
            }
        }
        return std::nullopt;
    }

private:
    /**
     * @brief Determines if a specific item VNUM corresponds to the requested potion type.
     * 
     * In Metin2, red potions (HP) typically have VNUMs between 27001 and 27003, 
     * and between 27007 and 27009 (auto-potions sometimes are 72723-72726, but normal red are 27001+).
     * Blue potions (SP) typically have VNUMs between 27004 and 27006.
     * This logic determines matching strictly based on known VNUM ranges.
     *
     * @param vnum The Virtual Number of the item to check.
     * @param type The required type of the potion.
     * @return true If the item matches the potion type.
     * @return false If the item does not match.
     */
    static bool IsPotionMatch(uint32_t vnum, PotionType type)
    {
        switch (type)
        {
            case PotionType::Red:
                // Red Potions: Small, Normal, Large and XXL types.
                return (vnum >= 27001 && vnum <= 27003) || (vnum >= 27007 && vnum <= 27009) || (vnum == 27051) || (vnum == 27201) || (vnum >= 72723 && vnum <= 72726);
            case PotionType::Blue:
                // Blue Potions: Small, Normal, Large and XXL types.
                return (vnum >= 27004 && vnum <= 27006) || (vnum == 27052) || (vnum == 27202) || (vnum >= 72727 && vnum <= 72730);
            default:
                return false;
        }
    }
};
