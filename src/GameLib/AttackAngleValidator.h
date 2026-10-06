#pragma once

#include <cstdint>
#include <cmath>
#include <expected>
#include <numbers>
#include <optional>

#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

/**
 * @file AttackAngleValidator.h
 * @brief Nowoczesny modul weryfikacji stozka ataku w C++23 dla domeny CombatMath.
 */

namespace GameLib::CombatMath {

    /**
     * @brief Struktura reprezentujaca punkt w przestrzeni 2D (plaszczyzna XY).
     */
    struct Point2D {
        float x;
        float y;
    };

    /**
     * @brief Struktura definiujaca stozek ataku (np. cios mieczem).
     */
    struct AttackCone {
        float radius;      ///< Zasięg maksymalny uderzenia.
        float arcDegrees;  ///< Całkowity kąt rozwarcia stozka w stopniach.
    };

    /**
     * @class AttackAngleValidator
     * @brief Klasa narzedziowa do weryfikacji pozycji celow wzgledem stozka ataku atakujacego.
     * 
     * Wykorzystuje nowoczesne standardy C++23 do precyzyjnych i bezpiecznych obliczen wektorowych.
     * Calkowicie oddzielona od mechanizmow GUI i silnika (Zero-Conflict).
     */
    class AttackAngleValidator {
    public:
        /**
         * @brief Weryfikuje czy cel znajduje sie w polu razenia stozka ataku.
         * 
         * Funkcja przeprowadza obliczenia z wykorzystaniem iloczynu skalarnego.
         * Wynik zwracany jest jako monada std::expected zapewniajaca 100% bezpieczenstwo obslugi bledow,
         * korzystajac z monadycznych wlasciwosci std::optional (.transform, .value_or).
         * 
         * @param attackerId Unikalny identyfikator atakujacego zdefiniowany przez silny typ.
         * @param targetId Unikalny identyfikator celu.
         * @param attackerPos Pozycja atakujacego na plaszczyznie 2D.
         * @param attackerRotDegrees Kierunek rotacji atakujacego w stopniach.
         * @param targetPos Pozycja celu na plaszczyznie 2D.
         * @param cone Definicja zasiegu i kata stozka ataku.
         * @return std::expected<void, EterBase::CombatError> Pusty stan oznaczajacy sukces (cel trafiony), 
         *         w przeciwnym razie blad (CombatError::OutOfRange).
         */
        [[nodiscard]] static std::expected<void, EterBase::CombatError> Validate(
            EterBase::EntityId attackerId,
            EterBase::EntityId targetId,
            const Point2D& attackerPos,
            float attackerRotDegrees,
            const Point2D& targetPos,
            const AttackCone& cone) noexcept
        {
            // Zwracamy dystans do kwadratu jesli jest w zasiegu promienia
            auto checkRange = [&]() -> std::optional<float> {
                const float dx = targetPos.x - attackerPos.x;
                const float dy = targetPos.y - attackerPos.y;
                const float distSq = (dx * dx) + (dy * dy);
                
                if (distSq > (cone.radius * cone.radius)) {
                    EterBase::ModernLogger::Debug(
                        "Attacker {} missed Target {} (Distance out of bounds)", 
                        attackerId.value(), targetId.value());
                    return std::nullopt;
                }
                return distSq;
            };

            // Sprawdzamy kat dla obiektow w waznym zasiegu
            auto checkAngle = [&](float distSq) -> std::expected<void, EterBase::CombatError> {
                if (distSq == 0.0f) {
                    return {}; // Cel idealnie naklada sie na atakujacego, traktowany jako trafiony
                }

                const float rad = attackerRotDegrees * std::numbers::pi_v<float> / 180.0f;
                
                // Konwersja rotacji na wektor kierunkowy (X = cos, Y = sin)
                const float dirX = std::cos(rad);
                const float dirY = std::sin(rad);

                const float dx = targetPos.x - attackerPos.x;
                const float dy = targetPos.y - attackerPos.y;
                const float dist = std::sqrt(distSq);
                
                const float targetDirX = dx / dist;
                const float targetDirY = dy / dist;

                // Iloczyn skalarny (Dot Product) obu znormalizowanych wektorow
                const float dotProduct = (dirX * targetDirX) + (dirY * targetDirY);
                
                // Polowa kata rozwarcia w radianach
                const float halfArcRad = (cone.arcDegrees / 2.0f) * std::numbers::pi_v<float> / 180.0f;
                const float minDot = std::cos(halfArcRad);

                if (dotProduct >= minDot) {
                    return {};
                }

                EterBase::ModernLogger::Debug(
                    "Attacker {} missed Target {} (Angle limit. Dot: {} < MinDot: {})", 
                    attackerId.value(), targetId.value(), dotProduct, minDot);

                return std::unexpected(EterBase::CombatError::OutOfRange);
            };

            // Wykorzystanie operacji monadycznych C++23 do eliminacji if-zagniezdzen
            return checkRange()
                .transform(checkAngle)
                .value_or(std::unexpected(EterBase::CombatError::OutOfRange));
        }
    };

} // namespace GameLib::CombatMath
