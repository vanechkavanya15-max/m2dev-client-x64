#pragma once

#include <immintrin.h>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <format>
#include <algorithm>
#include "MathSIMD.h"
#include "Result.h"

namespace Client::Core {

// Definicje bledow dla sprawdzania granic
enum class BoundsError : uint8_t {
    EmptySpan,
    OutputBufferTooSmall,
    InvalidRayDirection
};

// Funkcja pomocnicza do konwersji bledu na tekst
[[nodiscard]] constexpr std::string_view to_string(BoundsError error) noexcept {
    switch (error) {
        case BoundsError::EmptySpan: return "Pusta rozpietosc wejsciowa (EmptySpan)";
        case BoundsError::OutputBufferTooSmall: return "Zbyt maly bufor wyjsciowy (OutputBufferTooSmall)";
        case BoundsError::InvalidRayDirection: return "Nieprawidlowy kierunek promienia - zero length (InvalidRayDirection)";
        default: return "Nieznany blad";
    }
}

// Struktura opisujaca promien do testow
struct alignas(16) Ray {
    Vec3 origin;
    Vec3 direction;
    Vec3 invDirection; // Zoptymalizowane dla metody Slab

    Ray() = default;

    static Result<Ray, BoundsError> Create(const Vec3& origin, const Vec3& dir) noexcept {
        float dirLengthSq = dir.Dot(dir);
        if (dirLengthSq < 1e-6f) {
            return std::unexpected(BoundsError::InvalidRayDirection);
        }

        Ray r;
        r.origin = origin;
        r.direction = dir;
        
        // Bezpieczne dzielenie dla inwersji
        r.invDirection.x = dir.x != 0.0f ? 1.0f / dir.x : 1e30f; // infinity approximation
        r.invDirection.y = dir.y != 0.0f ? 1.0f / dir.y : 1e30f;
        r.invDirection.z = dir.z != 0.0f ? 1.0f / dir.z : 1e30f;
        r.invDirection.w = 0.0f;
        
        return r;
    }
};

// Klasa odpowiedzialna za wektorowe testy kolizji (SSE2)
class SIMDBoundsChecker {
public:
    // Szybki test przeciecia dwoch prostopadloscianow wyrownanych do osi (AABB)
    [[nodiscard]] static bool Intersects(const BoundingBox& a, const BoundingBox& b) noexcept {
        // Zalozenie: min <= max dla obu AABB
        // Uzywamy instrukcji SSE2 (wymaganych przez wymogi x86)
        // Uzywamy _mm_loadu_ps ze wzgledow bezpieczenstwa, by zapobiec Segfault jesli Vec3 nie bedzie strict-aligned.
        __m128 aMin = _mm_loadu_ps(a.min.v);
        __m128 aMax = _mm_loadu_ps(a.max.v);
        __m128 bMin = _mm_loadu_ps(b.min.v);
        __m128 bMax = _mm_loadu_ps(b.max.v);

        // a.max >= b.min -> m1
        __m128 m1 = _mm_cmpge_ps(aMax, bMin);
        // a.min <= b.max -> m2
        __m128 m2 = _mm_cmple_ps(aMin, bMax);
        
        // Koniunkcja (AND) warunkow dla x, y, z
        __m128 res = _mm_and_ps(m1, m2);
        
        // Ekstrakcja 3 bitow z maski znakow (x,y,z - w pomijamy maskujac przez 0x7)
        int mask = _mm_movemask_ps(res);
        return (mask & 0x7) == 0x7;
    }

    // Przeciecie promienia z prostopadloscianem wyrownanym do osi (AABB) metoda Slab (z SSE2)
    [[nodiscard]] static bool Intersects(const Ray& ray, const BoundingBox& box, float& outDistance) noexcept {
        __m128 boxMin = _mm_loadu_ps(box.min.v);
        __m128 boxMax = _mm_loadu_ps(box.max.v);
        __m128 rayOrigin = _mm_loadu_ps(ray.origin.v);
        __m128 rayInvDir = _mm_loadu_ps(ray.invDirection.v);

        // t1 = (box.min - ray.origin) * ray.invDir
        __m128 t1 = _mm_mul_ps(_mm_sub_ps(boxMin, rayOrigin), rayInvDir);
        // t2 = (box.max - ray.origin) * ray.invDir
        __m128 t2 = _mm_mul_ps(_mm_sub_ps(boxMax, rayOrigin), rayInvDir);

        // tMin = min(t1, t2)
        __m128 tMinVec = _mm_min_ps(t1, t2);
        // tMax = max(t1, t2)
        __m128 tMaxVec = _mm_max_ps(t1, t2);

        alignas(16) float tMin[4];
        alignas(16) float tMax[4];
        _mm_store_ps(tMin, tMinVec);
        _mm_store_ps(tMax, tMaxVec);

        // Szukamy globalnego tNear i tFar
        float tNear = std::max({tMin[0], tMin[1], tMin[2]});
        float tFar = std::min({tMax[0], tMax[1], tMax[2]});

        if (tNear > tFar || tFar < 0.0f) {
            return false;
        }

        outDistance = tNear > 0.0f ? tNear : tFar;
        return true;
    }

    // Masowe sprawdzenie kolizji glownego AABB z zadanymi granicami (tzw. batching). 
    // Zwraca ilosc wykrytych kolizji, wypelnia bufor wyjsciowy indeksami.
    static Result<size_t, BoundsError> BatchIntersects(
        const BoundingBox& mainBox,
        std::span<const BoundingBox> targetBoxes,
        std::span<size_t> outCollisionIndices) noexcept 
    {
        if (targetBoxes.empty()) {
            return std::unexpected(BoundsError::EmptySpan);
        }

        size_t collisionCount = 0;
        for (size_t i = 0; i < targetBoxes.size(); ++i) {
            if (Intersects(mainBox, targetBoxes[i])) {
                if (collisionCount >= outCollisionIndices.size()) {
                    return std::unexpected(BoundsError::OutputBufferTooSmall);
                }
                outCollisionIndices[collisionCount++] = i;
            }
        }
        
        return collisionCount;
    }

    // Masowe sprawdzanie promienia z zadanymi granicami.
    // Zwraca ilosc trafien i zapisuje je do bufora outHitIndices.
    static Result<size_t, BoundsError> BatchRayIntersects(
        const Ray& ray,
        std::span<const BoundingBox> targetBoxes,
        std::span<size_t> outHitIndices) noexcept 
    {
        if (targetBoxes.empty()) {
            return std::unexpected(BoundsError::EmptySpan);
        }

        size_t hitCount = 0;
        float dummyDist;
        for (size_t i = 0; i < targetBoxes.size(); ++i) {
            if (Intersects(ray, targetBoxes[i], dummyDist)) {
                if (hitCount >= outHitIndices.size()) {
                    return std::unexpected(BoundsError::OutputBufferTooSmall);
                }
                outHitIndices[hitCount++] = i;
            }
        }
        
        return hitCount;
    }
};

} // namespace Client::Core

// Specjalizacja formatowania dla błędu domeny
template <>
struct std::formatter<Client::Core::BoundsError> : std::formatter<std::string_view> {
    auto format(Client::Core::BoundsError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
