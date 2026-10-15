#pragma once

#include <expected>
#include <cmath>
#include <numbers>

namespace Client::Formulas {

// Blad walidacji kata ataku
enum class AttackAngleError {
    InvalidMaxAngle,
    ZeroDirectionVector,
    TargetOutsideCone
};

// Struktury pomocnicze
struct Position2D {
    float x;
    float y;
};

struct Vector2D {
    float x;
    float y;
};

class AttackAngleValidator {
public:
    // Glowna funkcja sprawdzajaca czy cel jest wewnatrz stozka ataku (w ujeciu 2D)
    // atakujacy_pozycja: pozycja postaci atakujacej
    // kierunek_ataku: wektor wyznaczajacy kierunek, w ktorym postac atakuje
    // cel_pozycja: pozycja obrywajacej postaci/celu
    // maksymalny_kat_stopnie: rozmiar stozka wyznaczony w stopniach (np. 120 oznacza rozpietosc 120 stopni)
    [[nodiscard]] static std::expected<void, AttackAngleError>
    IsTargetInAttackCone(const Position2D& atakujacy_pozycja,
                         const Vector2D& kierunek_ataku,
                         const Position2D& cel_pozycja,
                         float maksymalny_kat_stopnie) noexcept {
        
        if (maksymalny_kat_stopnie <= 0.0f || maksymalny_kat_stopnie > 360.0f) {
            return std::unexpected(AttackAngleError::InvalidMaxAngle);
        }

        float k_len2 = kierunek_ataku.x * kierunek_ataku.x + kierunek_ataku.y * kierunek_ataku.y;
        if (k_len2 == 0.0f) {
            return std::unexpected(AttackAngleError::ZeroDirectionVector);
        }

        Vector2D wektor_celu = { cel_pozycja.x - atakujacy_pozycja.x, cel_pozycja.y - atakujacy_pozycja.y };
        float c_len2 = wektor_celu.x * wektor_celu.x + wektor_celu.y * wektor_celu.y;

        if (c_len2 == 0.0f) {
            // Ten sam punkt traktujemy jako mieszczacy sie w kacie ataku (przypadek minimalnej odleglosci)
            return {};
        }

        // Iloczyn skalarny dwoch wektorow
        float iloczyn_skalarny = wektor_celu.x * kierunek_ataku.x + wektor_celu.y * kierunek_ataku.y;

        // Cosinus kata miedzy wektorami to (u * v) / (|u| * |v|)
        // Nie musimy liczyc acos, porownujemy cosinusy (w pamieci o monotonicznosci funkcji cosinus w przedziale 0-180)
        
        float cosinus_docelowy = iloczyn_skalarny / std::sqrt(c_len2 * k_len2);
        
        // Prawa strona rownania to polowa maksymalnego kata zamachu (bo stozek idzie w dwie strony od glownego wektora)
        // Zamiana stopni na radiany
        float max_kat_radiany = (maksymalny_kat_stopnie / 2.0f) * (std::numbers::pi_v<float> / 180.0f);
        float limit_cosinusa = std::cos(max_kat_radiany);

        if (cosinus_docelowy >= limit_cosinusa) {
            return {};
        } else {
            return std::unexpected(AttackAngleError::TargetOutsideCone);
        }
    }
};

} // namespace Client::Formulas
