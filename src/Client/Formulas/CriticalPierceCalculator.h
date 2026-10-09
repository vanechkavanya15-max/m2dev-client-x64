#pragma once

#include <cstdint>
#include <expected>
#include <algorithm>

namespace Client::Formulas {

    // Kody bledow dla kalkulatora szansy na uderzenie krytyczne i przeszywajace
    enum class CriticalPierceError : uint8_t {
        InvalidRandomRoll,
        InvalidBaseDamage
    };

    // Struktura wejsciowa agregujaca parametry atakujacego i obroncy
    struct CriticalPierceContext {
        int32_t attackerCriticalChance{0};
        int32_t defenderCriticalResistance{0};
        int32_t attackerPierceChance{0};
        int32_t defenderPierceResistance{0};
    };

    // Struktura wyjsciowa opisujaca wlasciwosci pojedynczego uderzenia
    struct HitProperties {
        bool isCritical{false};
        bool isPierce{false};
        int32_t damageMultiplier{100}; // Wartosc w procentach (100 = 100% oryginalnych obrazen)
    };

    // Klasa narzedziowa implementujaca czysta matematyke dla ciosow krytycznych i przeszywajacych
    // Zaprojektowana zgodnie z zasada Zero-Conflict oraz Stateless Functions
    class CriticalPierceCalculator {
    public:
        // Brak mozliwosci instancjonowania - wszystkie metody sa statyczne
        CriticalPierceCalculator() = delete;

        // Glowna metoda obliczajaca rezultat ciosu.
        // Oczekuje rzutow losowych w przedziale [1, 100].
        [[nodiscard]] static constexpr std::expected<HitProperties, CriticalPierceError> CalculateHit(
            const CriticalPierceContext& context,
            int32_t randomCriticalRoll,
            int32_t randomPierceRoll) noexcept
        {
            if (randomCriticalRoll < 1 || randomCriticalRoll > 100 ||
                randomPierceRoll < 1 || randomPierceRoll > 100) 
            {
                return std::unexpected(CriticalPierceError::InvalidRandomRoll);
            }

            const int32_t netCriticalChance = std::clamp(
                context.attackerCriticalChance - context.defenderCriticalResistance, 
                0, 100);
                
            const int32_t netPierceChance = std::clamp(
                context.attackerPierceChance - context.defenderPierceResistance, 
                0, 100);

            HitProperties properties{};
            properties.isCritical = randomCriticalRoll <= netCriticalChance;
            properties.isPierce = randomPierceRoll <= netPierceChance;

            // W przypadku uderzenia krytycznego mnoznik obrazen wzrasta do 200%
            properties.damageMultiplier = properties.isCritical ? 200 : 100;

            return properties;
        }

        // Funkcje pomocnicze do weryfikacji szans (np. dla potrzeb interfejsu uzytkownika UI)
        
        [[nodiscard]] static constexpr int32_t GetNetCriticalChance(int32_t attackChance, int32_t defResist) noexcept {
            return std::clamp(attackChance - defResist, 0, 100);
        }

        [[nodiscard]] static constexpr int32_t GetNetPierceChance(int32_t attackChance, int32_t defResist) noexcept {
            return std::clamp(attackChance - defResist, 0, 100);
        }
    };

} // namespace Client::Formulas
