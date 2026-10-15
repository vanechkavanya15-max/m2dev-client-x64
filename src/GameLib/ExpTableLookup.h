#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace Metin2::CombatMath {

/**
 * @brief Utility class providing experience points required for leveling up.
 * 
 * Provides an array and helper methods to fetch the required experience points
 * to transition from a given level to the next, up to the maximum level limit.
 */
class ExpTableLookup {
public:
    /**
     * @brief Maximum reachable character level.
     */
    static constexpr uint8_t MAX_LEVEL = 120;
    
    /**
     * @brief Gets the experience points required to advance from the specified level to the next.
     * 
     * @param level Current player level (1 to 120).
     * @return std::optional<uint32_t> The required EXP to reach the next level. 
     *         Returns std::nullopt if the level is invalid (0) or if the player has reached MAX_LEVEL.
     */
    static std::optional<uint32_t> GetRequiredExpForLevel(uint8_t level) {
        if (level == 0 || level >= MAX_LEVEL) {
            return std::nullopt;
        }
        return EXP_TABLE[level];
    }
    
    /**
     * @brief Gets the total accumulated experience points required to reach the specified level from level 1.
     * 
     * @param level Target player level (1 to 120).
     * @return std::optional<uint64_t> The total EXP required.
     *         Returns std::nullopt if the level is invalid (0 or greater than MAX_LEVEL).
     */
    static std::optional<uint64_t> GetTotalExpForLevel(uint8_t level) {
        if (level == 0 || level > MAX_LEVEL) {
            return std::nullopt;
        }
        
        uint64_t totalExp = 0;
        for (uint8_t i = 1; i < level; ++i) {
            totalExp += EXP_TABLE[i];
        }
        return totalExp;
    }

private:
    /**
     * @brief Static table containing required experience points per level.
     * Index represents the current level. The value is the EXP required to reach level + 1.
     * Index 0 is unused (level 0 doesn't exist).
     * Index MAX_LEVEL (120) is 0 because no more EXP can be gained.
     */
    static constexpr std::array<uint32_t, MAX_LEVEL + 1> EXP_TABLE = {
        0, 300, 800, 1500, 2500, 4300, 7200, 11000, 17000, 24000, 33000, 
        43000, 58000, 76000, 100000, 130000, 169000, 219000, 283000, 365000, 472000, 
        610000, 705000, 815000, 942000, 1088000, 1256000, 1449000, 1671000, 1926000, 2219000, 
        2555000, 2800000, 3000000, 3200000, 3400000, 3700000, 4000000, 4400000, 5000000, 5600000, 
        6400000, 7300000, 8400000, 9600000, 11000000, 12600000, 14400000, 16400000, 18600000, 21000000, 
        23600000, 26400000, 29400000, 32600000, 36000000, 39600000, 43400000, 47400000, 51600000, 56000000, 
        60600000, 65400000, 70400000, 75600000, 81000000, 86600000, 92400000, 98400000, 104600000, 111000000, 
        117600000, 124400000, 131400000, 138600000, 146000000, 153600000, 161400000, 169400000, 177600000, 186000000, 
        194600000, 203400000, 212400000, 221600000, 231000000, 240600000, 250400000, 260400000, 270600000, 281000000, 
        291600000, 302400000, 313400000, 324600000, 336000000, 347600000, 359400000, 371400000, 383600000, 396000000, 
        408600000, 421400000, 434400000, 447600000, 461000000, 474600000, 488400000, 502400000, 516600000, 531000000, 
        545600000, 560400000, 575400000, 590600000, 606000000, 621600000, 637400000, 653400000, 669600000, 0
    };
};

} // namespace Metin2::CombatMath
