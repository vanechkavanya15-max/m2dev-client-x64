#pragma once

#include <expected>
#include <array>
#include <cstdint>
#include <limits>

namespace Client::Formulas {

    enum class PlayerClass : uint8_t {
        Warrior = 0,
        Ninja = 1,
        Sura = 2,
        Shaman = 3
    };

    enum class SkillDamageError : uint8_t {
        InvalidSkillIndex,
        InvalidLevel,
        MultiplicationOverflow,
        InvalidClass
    };

    class SkillDamageScaling {
    public:
        // Maksymalny wspierany poziom skilla to np. 40 (dla potrzeb P)
        static constexpr uint8_t MAX_SKILL_LEVEL = 40;
        
        // Mnozniki podstawowe (przykladowe zbalansowane wartosci w % gdzie 100 to 1.0)
        static constexpr std::array<uint32_t, 4> CLASS_MULTIPLIERS = {
            120, // Warrior
            110, // Ninja
            115, // Sura
            105  // Shaman
        };
        
        static constexpr std::array<uint32_t, 40> LEVEL_SCALARS = []() {
            std::array<uint32_t, 40> arr{};
            for (uint8_t i = 0; i < 40; ++i) {
                // Poziom 1 zaczyna od 100%, kazdy kolejny poziom dodaje 5% 
                arr[i] = 100 + (i * 5); 
            }
            return arr;
        }();

        // Czysta bezstanowa funkcja skalujaca obrazenia umiejetnosci bazowe 
        [[nodiscard]] static constexpr std::expected<uint64_t, SkillDamageError> CalculateScaledDamage(
            PlayerClass playerClass,
            uint32_t baseDamage,
            uint8_t skillLevel
        ) noexcept {
            if (skillLevel == 0 || skillLevel > MAX_SKILL_LEVEL) {
                return std::unexpected(SkillDamageError::InvalidLevel);
            }

            const uint8_t classIdx = static_cast<uint8_t>(playerClass);
            if (classIdx >= CLASS_MULTIPLIERS.size()) {
                return std::unexpected(SkillDamageError::InvalidClass);
            }

            const uint32_t classMul = CLASS_MULTIPLIERS[classIdx];
            const uint32_t levelMul = LEVEL_SCALARS[skillLevel - 1];

            // Obliczenia mnoznikow
            // baseDamage * (classMul / 100.0) * (levelMul / 100.0)
            
            // Sprawdzenie przepelnien mnozenia bazowego
            uint64_t step1 = static_cast<uint64_t>(baseDamage) * classMul;
            if (step1 < baseDamage) {
                return std::unexpected(SkillDamageError::MultiplicationOverflow);
            }
            step1 /= 100;

            uint64_t step2 = step1 * levelMul;
            if (step1 != 0 && step2 / step1 != levelMul) {
                return std::unexpected(SkillDamageError::MultiplicationOverflow);
            }
            step2 /= 100;

            return step2;
        }
    };

} // namespace Client::Formulas
