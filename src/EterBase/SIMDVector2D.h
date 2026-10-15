#pragma once

#include <immintrin.h>
#include <cmath>
#include <expected>
#include <optional>
#include <string_view>

#include "Result.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @brief A 2D vector class optimized with SIMD (SSE/AVX) instructions for fast mathematical operations.
 * 
 * This class provides standard vector operations such as addition, subtraction, scalar multiplication,
 * dot product, distance calculations, and normalization. Methods that can fail (e.g., normalizing a 
 * zero-length vector) use C++23 monadic types like std::expected or std::optional to prevent undefined behavior.
 */
class SIMDVector2D {
public:
    /**
     * @brief Default constructor. Initializes the vector to (0.0f, 0.0f).
     */
    constexpr SIMDVector2D() noexcept : m_data{0.0f, 0.0f, 0.0f, 0.0f} {}

    /**
     * @brief Constructor from two float components.
     * @param x The X component of the vector.
     * @param y The Y component of the vector.
     */
    SIMDVector2D(float x, float y) noexcept {
        m_vec = _mm_set_ps(0.0f, 0.0f, y, x);
    }

    /**
     * @brief Constructor from a __m128 intrinsic.
     * @param v The SIMD intrinsic value.
     */
    explicit SIMDVector2D(__m128 v) noexcept : m_vec(v) {}

    /**
     * @brief Retrieves the X component.
     * @return The X component.
     */
    [[nodiscard]] float GetX() const noexcept { return m_data[0]; }

    /**
     * @brief Retrieves the Y component.
     * @return The Y component.
     */
    [[nodiscard]] float GetY() const noexcept { return m_data[1]; }

    /**
     * @brief Adds another vector to this vector.
     * @param other The other vector to add.
     * @return A new vector representing the sum.
     */
    [[nodiscard]] SIMDVector2D operator+(const SIMDVector2D& other) const noexcept {
        return SIMDVector2D(_mm_add_ps(m_vec, other.m_vec));
    }

    /**
     * @brief Subtracts another vector from this vector.
     * @param other The other vector to subtract.
     * @return A new vector representing the difference.
     */
    [[nodiscard]] SIMDVector2D operator-(const SIMDVector2D& other) const noexcept {
        return SIMDVector2D(_mm_sub_ps(m_vec, other.m_vec));
    }

    /**
     * @brief Multiplies the vector by a scalar.
     * @param scalar The scalar value to multiply by.
     * @return A new vector representing the scaled result.
     */
    [[nodiscard]] SIMDVector2D operator*(float scalar) const noexcept {
        __m128 s = _mm_set1_ps(scalar);
        return SIMDVector2D(_mm_mul_ps(m_vec, s));
    }

    /**
     * @brief Multiplies the vector by a scalar (friend function).
     * @param scalar The scalar value.
     * @param v The vector.
     * @return A new vector representing the scaled result.
     */
    friend SIMDVector2D operator*(float scalar, const SIMDVector2D& v) noexcept {
        return v * scalar;
    }

    /**
     * @brief Calculates the dot product of this vector and another.
     * @param other The other vector.
     * @return The dot product as a float.
     */
    [[nodiscard]] float DotProduct(const SIMDVector2D& other) const noexcept {
        __m128 mul = _mm_mul_ps(m_vec, other.m_vec);
#ifdef __SSE3__
        __m128 sum = _mm_hadd_ps(mul, mul);
        sum = _mm_hadd_ps(sum, sum);
        return _mm_cvtss_f32(sum);
#else
        alignas(16) float res[4];
        _mm_store_ps(res, mul);
        return res[0] + res[1];
#endif
    }

    /**
     * @brief Calculates the squared length (magnitude) of the vector.
     * @return The squared length.
     */
    [[nodiscard]] float LengthSquared() const noexcept {
        return DotProduct(*this);
    }

    /**
     * @brief Calculates the length (magnitude) of the vector.
     * @return The length.
     */
    [[nodiscard]] float Length() const noexcept {
        return std::sqrt(LengthSquared());
    }

    /**
     * @brief Normalizes the vector, converting it to a unit vector.
     * @return A valid SIMDVector2D if successful, or an error if the vector length is zero.
     */
    [[nodiscard]] std::expected<SIMDVector2D, std::string_view> Normalize() const noexcept {
        float lenSq = LengthSquared();
        if (lenSq < 1e-8f) {
            ModernLogger::Warn("Attempted to normalize a zero-length vector");
            return std::unexpected("Cannot normalize zero-length vector");
        }
        
        float invLen = 1.0f / std::sqrt(lenSq);
        __m128 s = _mm_set1_ps(invLen);
        return SIMDVector2D(_mm_mul_ps(m_vec, s));
    }

    /**
     * @brief Calculates the distance to another vector.
     * @param other The target vector.
     * @return The Euclidean distance between the two vectors.
     */
    [[nodiscard]] float DistanceTo(const SIMDVector2D& other) const noexcept {
        return (*this - other).Length();
    }

    /**
     * @brief Calculates the squared distance to another vector.
     * @param other The target vector.
     * @return The squared distance between the two vectors.
     */
    [[nodiscard]] float DistanceSquaredTo(const SIMDVector2D& other) const noexcept {
        return (*this - other).LengthSquared();
    }

private:
    union {
        __m128 m_vec;
        float m_data[4];
    };
};

} // namespace EterBase
