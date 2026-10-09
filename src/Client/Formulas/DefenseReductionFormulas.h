#pragma once

#include <cstdint>
#include <algorithm>

namespace Client::Formulas {

/**
 * @brief Klasa z bezstanowymi, matematycznymi formulami dla redukcji obrazen fizycznych i magicznych.
 */
class DefenseReductionFormulas {
public:
    /**
     * @brief Oblicza zredukowane obrazenia na podstawie wartosci ataku i odpornosci procentowej.
     * @param baseDamage Obrazenia poczatkowe.
     * @param resistancePercent Odpornosc procentowa (0-100%).
     * @return Zredukowane obrazenia (nigdy nie mniejsze niz 0).
     */
    [[nodiscard]] static constexpr int32_t CalculateReducedDamage(int32_t baseDamage, int32_t resistancePercent) noexcept {
        if (baseDamage <= 0) {
            return 0;
        }

        const int32_t clampedResistance = std::clamp(resistancePercent, 0, 100);
        
        // Zabezpieczenie przed przepelnieniem dla duzych wartosci damage
        // Wymagamy uint64_t zeby uniknac overflow
        const uint64_t damageU64 = static_cast<uint64_t>(baseDamage);
        const uint64_t reductionU64 = (damageU64 * static_cast<uint64_t>(clampedResistance)) / 100ULL;
        
        return static_cast<int32_t>(damageU64 - reductionU64);
    }
};

} // namespace Client::Formulas
