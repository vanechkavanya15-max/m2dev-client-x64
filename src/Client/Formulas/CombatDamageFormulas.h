#pragma once

#include <cstdint>
#include <expected>
#include <algorithm>

namespace Client::Formulas {

// Kody bledow kalkulacji obrazen
enum class DamageError : uint8_t {
    NegativeMultiplier,
    NegativeAttack,
    NegativeDefense,
    InvalidResist
};

// Modul zawierajacy czyste funkcje do kalkulacji obrazen (Pure Math).
// Odporny na bledy, dziala bezstanowo (bez zadnych zaleznosci klas).
namespace CombatDamageFormulas {

    // Kalkulacja obrazen fizycznych.
    // Zabezpieczone przed ujemnym atakiem, obrona oraz blednymi mnoznikami.
    [[nodiscard]] constexpr std::expected<int64_t, DamageError> CalculatePhysicalDamage(
        int64_t attack, int64_t defense, int64_t resistancePercent, double multiplier = 1.0) noexcept 
    {
        if (attack < 0) return std::unexpected(DamageError::NegativeAttack);
        if (defense < 0) return std::unexpected(DamageError::NegativeDefense);
        if (multiplier < 0.0) return std::unexpected(DamageError::NegativeMultiplier);
        if (resistancePercent < 0 || resistancePercent > 100) return std::unexpected(DamageError::InvalidResist);

        int64_t baseDamage = attack - defense;
        if (baseDamage < 0) {
            baseDamage = 0;
        }

        const double damageAfterResist = static_cast<double>(baseDamage) * (100.0 - static_cast<double>(resistancePercent)) / 100.0;
        const double finalDamage = damageAfterResist * multiplier;

        return static_cast<int64_t>(finalDamage);
    }

    // Kalkulacja obrazen magicznych. 
    // Magia ignoruje bezposredni pancerz, dziala tylko na odpornosc magiczna.
    [[nodiscard]] constexpr std::expected<int64_t, DamageError> CalculateMagicDamage(
        int64_t attack, int64_t magicResistancePercent, double multiplier = 1.0) noexcept 
    {
        if (attack < 0) return std::unexpected(DamageError::NegativeAttack);
        if (multiplier < 0.0) return std::unexpected(DamageError::NegativeMultiplier);
        if (magicResistancePercent < 0 || magicResistancePercent > 100) return std::unexpected(DamageError::InvalidResist);

        const double damageAfterResist = static_cast<double>(attack) * (100.0 - static_cast<double>(magicResistancePercent)) / 100.0;
        const double finalDamage = damageAfterResist * multiplier;

        return static_cast<int64_t>(finalDamage);
    }

    // Kalkulacja obrazen krytycznych.
    // Zwieksza podstawowe obrazenia, domyslnie dwukrotnie (x2.0).
    [[nodiscard]] constexpr std::expected<int64_t, DamageError> CalculateCriticalDamage(
        int64_t baseDamage, double criticalMultiplier = 2.0) noexcept 
    {
        if (baseDamage < 0) return std::unexpected(DamageError::NegativeAttack);
        if (criticalMultiplier < 1.0) return std::unexpected(DamageError::NegativeMultiplier);

        const double finalDamage = static_cast<double>(baseDamage) * criticalMultiplier;

        return static_cast<int64_t>(finalDamage);
    }

    // Kalkulacja obrazen z przebiciem pancerza (penetration).
    // Uderzenie calkowicie ignoruje obrone pancerza celu, uzywajac wylacznie redukcji procentowych.
    [[nodiscard]] constexpr std::expected<int64_t, DamageError> CalculatePenetrationDamage(
        int64_t attack, int64_t resistancePercent, double multiplier = 1.0) noexcept 
    {
        if (attack < 0) return std::unexpected(DamageError::NegativeAttack);
        if (multiplier < 0.0) return std::unexpected(DamageError::NegativeMultiplier);
        if (resistancePercent < 0 || resistancePercent > 100) return std::unexpected(DamageError::InvalidResist);

        const double damageAfterResist = static_cast<double>(attack) * (100.0 - static_cast<double>(resistancePercent)) / 100.0;
        const double finalDamage = damageAfterResist * multiplier;

        return static_cast<int64_t>(finalDamage);
    }

} // namespace CombatDamageFormulas
} // namespace Client::Formulas
