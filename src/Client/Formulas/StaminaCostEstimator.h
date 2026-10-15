#pragma once

#include <expected>
#include <cstdint>

namespace Client::Formulas {

/**
 * @brief Typ bledu dla kalkulacji staminy.
 */
enum class StaminaError : uint8_t {
    None = 0,
    NegativeTimeDelta,
    NegativeSpeed,
    InvalidMultiplier,
    CalculationOverflow,
    InvalidRate
};

/**
 * @brief Parametry wejsciowe dla zuzycia staminy.
 */
struct StaminaConsumptionParams {
    float timeDelta = 0.0f;
    float movementSpeed = 0.0f;
    float baseConsumptionRate = 10.0f; // zuzycie bazowe na sekunde
    float speedThreshold = 300.0f;     // prog predkosci, po ktorym zuzycie rosnie
    float highSpeedMultiplier = 1.5f;  // mnoznik dla wysokiej predkosci
};

/**
 * @brief Parametry wejsciowe dla regeneracji staminy.
 */
struct StaminaRegenerationParams {
    float timeDelta = 0.0f;
    bool isResting = false;
    float baseRegenRate = 5.0f; // odzyskiwanie bazowe na sekunde
    float restMultiplier = 2.0f; // mnoznik podczas odpoczynku
};

/**
 * @brief Bezstanowa klasa matematyczna obliczajaca zuzycie i regeneracje staminy.
 */
class StaminaCostEstimator {
public:
    /**
     * @brief Oblicza ilosc staminy zuzytej podczas ruchu.
     * @param params Parametry ruchu i czasu.
     * @return Zwraca wartosc zuzytej staminy lub blad.
     */
    [[nodiscard]] static constexpr std::expected<float, StaminaError> CalculateConsumption(const StaminaConsumptionParams& params) noexcept {
        if (params.timeDelta < 0.0f) {
            return std::unexpected(StaminaError::NegativeTimeDelta);
        }
        if (params.movementSpeed < 0.0f) {
            return std::unexpected(StaminaError::NegativeSpeed);
        }
        if (params.highSpeedMultiplier < 0.0f) {
            return std::unexpected(StaminaError::InvalidMultiplier);
        }
        if (params.baseConsumptionRate < 0.0f) {
            return std::unexpected(StaminaError::InvalidRate);
        }

        if (params.timeDelta == 0.0f || params.movementSpeed == 0.0f) {
            return 0.0f; // Brak ruchu lub czasu
        }

        float multiplier = 1.0f;
        if (params.movementSpeed > params.speedThreshold) {
            multiplier = params.highSpeedMultiplier;
        }

        const float consumption = params.baseConsumptionRate * params.timeDelta * multiplier;

        if (consumption > 1000000.0f) { // Zapobieganie przesadnym wartosciom
            return std::unexpected(StaminaError::CalculationOverflow);
        }

        return consumption;
    }

    /**
     * @brief Oblicza ilosc odzyskanej staminy podczas spoczynku lub normalnego stania.
     * @param params Parametry regeneracji i czasu.
     * @return Zwraca wartosc odzyskanej staminy lub blad.
     */
    [[nodiscard]] static constexpr std::expected<float, StaminaError> CalculateRegeneration(const StaminaRegenerationParams& params) noexcept {
        if (params.timeDelta < 0.0f) {
            return std::unexpected(StaminaError::NegativeTimeDelta);
        }
        if (params.restMultiplier < 0.0f) {
            return std::unexpected(StaminaError::InvalidMultiplier);
        }
        if (params.baseRegenRate < 0.0f) {
            return std::unexpected(StaminaError::InvalidRate);
        }

        if (params.timeDelta == 0.0f) {
            return 0.0f;
        }

        float rate = params.baseRegenRate;
        if (params.isResting) {
            rate *= params.restMultiplier;
        }

        const float regen = rate * params.timeDelta;

        if (regen > 1000000.0f) {
            return std::unexpected(StaminaError::CalculationOverflow);
        }

        return regen;
    }
};

} // namespace Client::Formulas
