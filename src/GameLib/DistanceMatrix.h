#pragma once

#include <cstdint>
#include <concepts>
#include <type_traits>

/**
 * @file DistanceMatrix.h
 * @brief Fast squared-distance calculator without costly sqrt operations for range checking.
 */

/**
 * @class DistanceMatrix
 * @brief Provides static methods to calculate squared distances and perform range checks.
 */
class DistanceMatrix
{
public:
    DistanceMatrix() = delete;
    ~DistanceMatrix() = delete;

    /**
     * @brief Calculates the squared distance between two 2D points.
     * 
     * @tparam T Arithmetic type of the coordinates.
     * @param x1 The X coordinate of the first point.
     * @param y1 The Y coordinate of the first point.
     * @param x2 The X coordinate of the second point.
     * @param y2 The Y coordinate of the second point.
     * @return T The squared distance.
     */
    template <typename T>
    requires std::is_arithmetic_v<T>
    [[nodiscard]] static constexpr T CalculateSquaredDistance2D(T x1, T y1, T x2, T y2) noexcept
    {
        const T dx = x2 - x1;
        const T dy = y2 - y1;
        return (dx * dx) + (dy * dy);
    }

    /**
     * @brief Calculates the squared distance between two 3D points.
     * 
     * @tparam T Arithmetic type of the coordinates.
     * @param x1 The X coordinate of the first point.
     * @param y1 The Y coordinate of the first point.
     * @param z1 The Z coordinate of the first point.
     * @param x2 The X coordinate of the second point.
     * @param y2 The Y coordinate of the second point.
     * @param z2 The Z coordinate of the second point.
     * @return T The squared distance.
     */
    template <typename T>
    requires std::is_arithmetic_v<T>
    [[nodiscard]] static constexpr T CalculateSquaredDistance3D(T x1, T y1, T z1, T x2, T y2, T z2) noexcept
    {
        const T dx = x2 - x1;
        const T dy = y2 - y1;
        const T dz = z2 - z1;
        return (dx * dx) + (dy * dy) + (dz * dz);
    }

    /**
     * @brief Checks if two 2D points are within a specified range.
     * 
     * @tparam T Arithmetic type of the coordinates.
     * @param x1 The X coordinate of the first point.
     * @param y1 The Y coordinate of the first point.
     * @param x2 The X coordinate of the second point.
     * @param y2 The Y coordinate of the second point.
     * @param range The maximum allowed distance between the two points.
     * @return true If the distance is less than or equal to the range.
     * @return false If the distance is strictly greater than the range.
     */
    template <typename T>
    requires std::is_arithmetic_v<T>
    [[nodiscard]] static constexpr bool IsInRange2D(T x1, T y1, T x2, T y2, T range) noexcept
    {
        return CalculateSquaredDistance2D(x1, y1, x2, y2) <= (range * range);
    }

    /**
     * @brief Checks if two 3D points are within a specified range.
     * 
     * @tparam T Arithmetic type of the coordinates.
     * @param x1 The X coordinate of the first point.
     * @param y1 The Y coordinate of the first point.
     * @param z1 The Z coordinate of the first point.
     * @param x2 The X coordinate of the second point.
     * @param y2 The Y coordinate of the second point.
     * @param z2 The Z coordinate of the second point.
     * @param range The maximum allowed distance between the two points.
     * @return true If the distance is less than or equal to the range.
     * @return false If the distance is strictly greater than the range.
     */
    template <typename T>
    requires std::is_arithmetic_v<T>
    [[nodiscard]] static constexpr bool IsInRange3D(T x1, T y1, T z1, T x2, T y2, T z2, T range) noexcept
    {
        return CalculateSquaredDistance3D(x1, y1, z1, x2, y2, z2) <= (range * range);
    }
};
