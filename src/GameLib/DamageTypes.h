#pragma once

#include <cstdint>
#include <string_view>
#include <stdexcept>

namespace game_lib {

/**
 * @brief Represents the various types of damage that can be applied in combat.
 * 
 * This enumeration uses a strong C++20 type with a fixed underlying uint8_t size.
 * It is completely decoupled from any GUI state and defines pure logical combat values.
 */
enum class DamageType : uint8_t {
    DAMAGE_NORMAL    = 0, ///< Standard combat damage without special effects.
    DAMAGE_CRITICAL  = 1, ///< Critical hit damage, usually resulting in a multiplier.
    DAMAGE_PENETRATE = 2, ///< Penetrating damage that bypasses defensive values.
    DAMAGE_POISON    = 3  ///< Damage over time caused by poisoning effects.
};

/**
 * @brief Converts a DamageType enum value to its string representation.
 *
 * @param type The DamageType value to convert.
 * @return std::string_view A lightweight string view representing the damage type name.
 * 
 * @throw std::invalid_argument If an unknown DamageType is provided.
 */
constexpr std::string_view ToString(DamageType type) {
    switch (type) {
        case DamageType::DAMAGE_NORMAL:
            return "DAMAGE_NORMAL";
        case DamageType::DAMAGE_CRITICAL:
            return "DAMAGE_CRITICAL";
        case DamageType::DAMAGE_PENETRATE:
            return "DAMAGE_PENETRATE";
        case DamageType::DAMAGE_POISON:
            return "DAMAGE_POISON";
        default:
            throw std::invalid_argument("Unknown DamageType provided");
    }
}

} // namespace game_lib
