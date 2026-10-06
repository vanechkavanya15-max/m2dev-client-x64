#ifndef USERINTERFACE_DOMAIN_VECTOR2D_H
#define USERINTERFACE_DOMAIN_VECTOR2D_H

#include <cmath>
#include <concepts>
#include <type_traits>
#include <cstdint>

namespace Domain {

/**
 * @brief A lightweight, standard C++20 2D vector structure.
 * 
 * This structure adheres to the single responsibility principle, 
 * focusing only on 2D mathematical operations for the movement domain.
 * It uses C++20 concepts to restrict the template type to arithmetic types.
 *
 * @tparam T The arithmetic type of the vector components (e.g., float, double, int32_t).
 */
template <typename T>
    requires std::is_arithmetic_v<T>
struct Vector2D {
    /** @brief The X component of the vector. */
    T x{0};

    /** @brief The Y component of the vector. */
    T y{0};

    /**
     * @brief Default constructor. Initializes components to 0.
     */
    constexpr Vector2D() noexcept = default;

    /**
     * @brief Parameterized constructor.
     * 
     * @param initialX Initial value for the X component.
     * @param initialY Initial value for the Y component.
     */
    constexpr Vector2D(T initialX, T initialY) noexcept 
        : x(initialX), y(initialY) {}

    /**
     * @brief Calculates the Euclidean distance to another Vector2D.
     * 
     * @param target The target vector to calculate the distance to.
     * @return The distance as a double.
     */
    [[nodiscard]] double DistanceTo(const Vector2D& target) const noexcept {
        const double deltaX = static_cast<double>(target.x - x);
        const double deltaY = static_cast<double>(target.y - y);
        return std::sqrt(deltaX * deltaX + deltaY * deltaY);
    }

    /**
     * @brief Calculates the angle (in radians) from this vector to another Vector2D.
     * 
     * @param target The target vector.
     * @return The angle in radians, ranging from -pi to pi, as a double.
     */
    [[nodiscard]] double AngleTo(const Vector2D& target) const noexcept {
        const double deltaX = static_cast<double>(target.x - x);
        const double deltaY = static_cast<double>(target.y - y);
        return std::atan2(deltaY, deltaX);
    }

    /**
     * @brief Performs linear interpolation between this vector and a target vector.
     * 
     * @param target The target vector to interpolate towards.
     * @param factor The interpolation factor, typically between 0.0 and 1.0.
     * @return A new Vector2D resulting from the interpolation.
     */
    [[nodiscard]] constexpr Vector2D Lerp(const Vector2D& target, double factor) const noexcept {
        return Vector2D(
            static_cast<T>(std::lerp(static_cast<double>(x), static_cast<double>(target.x), factor)),
            static_cast<T>(std::lerp(static_cast<double>(y), static_cast<double>(target.y), factor))
        );
    }
};

/** @brief Alias for a Vector2D with float components. */
using Vector2Df = Vector2D<float>;

/** @brief Alias for a Vector2D with int32_t components. */
using Vector2Di = Vector2D<int32_t>;

} // namespace Domain

#endif // USERINTERFACE_DOMAIN_VECTOR2D_H
