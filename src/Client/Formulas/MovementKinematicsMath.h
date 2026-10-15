#pragma once

#include <expected>
#include <cmath>
#include <numbers>
#include <tuple>
#include <type_traits>

namespace Client::Formulas::MovementKinematicsMath {

    enum class MathError {
        ZeroLengthVector,
        ConstexprNotSupported // Dodano nowy blad dla operacji niemozliwych w constexpr
    };

    struct Vector3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr Vector3() noexcept = default;
        constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

        constexpr Vector3 operator-(const Vector3& other) const noexcept {
            return {x - other.x, y - other.y, z - other.z};
        }

        constexpr Vector3 operator+(const Vector3& other) const noexcept {
            return {x + other.x, y + other.y, z + other.z};
        }

        constexpr Vector3 operator*(float scalar) const noexcept {
            return {x * scalar, y * scalar, z * scalar};
        }
        
        constexpr bool operator==(const Vector3& other) const noexcept = default;
    };

    constexpr float SquaredLength(const Vector3& v) noexcept {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    constexpr float Length(const Vector3& v) noexcept {
        if (std::is_constant_evaluated()) {
            // constexpr sqrt fallback dla C++20/C++23
            #if defined(__clang__) || defined(__GNUC__)
                return __builtin_sqrt(SquaredLength(v));
            #else
                return 0.0f; // Brak wsparcia w constexpr dla innych kompilatorow w C++20
            #endif
        } else {
            return std::sqrt(SquaredLength(v));
        }
    }

    constexpr std::expected<Vector3, MathError> Normalize(const Vector3& v) noexcept {
        const float lenSq = SquaredLength(v);
        if (lenSq <= 1e-6f) {
            return std::unexpected(MathError::ZeroLengthVector);
        }
        const float len = Length(v);
        return Vector3{v.x / len, v.y / len, v.z / len};
    }

    constexpr std::expected<Vector3, MathError> Direction(const Vector3& from, const Vector3& to) noexcept {
        return Normalize(to - from);
    }

    constexpr float DotProduct(const Vector3& a, const Vector3& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    constexpr Vector3 CrossProduct(const Vector3& a, const Vector3& b) noexcept {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    // Kat w stopniach miedzy dwoma znormalizowanymi wektorami
    constexpr std::expected<float, MathError> AngleBetween(const Vector3& dir1, const Vector3& dir2) noexcept {
        // Zakladamy, ze dir1 i dir2 sa znormalizowane
        const float dot = DotProduct(dir1, dir2);
        // Ograniczenie do [-1.0, 1.0], aby uniknac bledow domeny dla funkcji acos
        const float clampedDot = dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot);
        
        if (std::is_constant_evaluated()) {
            return std::unexpected(MathError::ConstexprNotSupported); 
        } else {
            return std::acos(clampedDot) * (180.0f / std::numbers::pi_v<float>);
        }
    }

    // Interpolacja liniowa rotacji w stopniach, wyszukujaca najkrotsza sciezke
    constexpr float InterpolateRotation(float startAngle, float targetAngle, float t) noexcept {
        float diff = targetAngle - startAngle;
        
        if (std::is_constant_evaluated()) {
            // Normalizacja roznicy do [-180, 180] z petla (fmod nie jest constexpr w c++20)
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
        } else {
            // Bezpieczniejsza i wydajniejsza wersja dla runtime w przypadku ogromnych wartosci (zabezpieczenie przed infinite loop)
            diff = std::fmod(diff, 360.0f);
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
        }
        
        return startAngle + diff * t;
    }

    // Obliczanie rotacji yaw na podstawie wektora kierunku
    constexpr std::expected<float, MathError> CalculateYawFromDirection(const Vector3& dir) noexcept {
        if (std::is_constant_evaluated()) {
            return std::unexpected(MathError::ConstexprNotSupported);
        } else {
            float angle = std::atan2(dir.y, dir.x) * (180.0f / std::numbers::pi_v<float>);
            if (angle < 0.0f) angle += 360.0f;
            return angle;
        }
    }

} // namespace Client::Formulas::MovementKinematicsMath
