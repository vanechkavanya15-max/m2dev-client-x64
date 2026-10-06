#pragma once

#include <cstdint>

/**
 * @brief Class responsible for validating map coordinates against defined boundaries.
 * 
 * This class ensures that given 2D coordinates (X, Y) fall within the valid
 * boundaries of a map. It supports setting explicit boundary limits and checking
 * coordinates against them, which is essential for preventing out-of-bounds 
 * navigation or rendering issues.
 */
class MapBoundaryChecker
{
public:
    /**
     * @brief Constructs a MapBoundaryChecker with default zero boundaries.
     */
    MapBoundaryChecker() = default;

    /**
     * @brief Constructs a MapBoundaryChecker with specific boundaries.
     * 
     * @param minX Minimum X coordinate allowed.
     * @param minY Minimum Y coordinate allowed.
     * @param maxX Maximum X coordinate allowed.
     * @param maxY Maximum Y coordinate allowed.
     */
    MapBoundaryChecker(float minX, float minY, float maxX, float maxY)
        : minX(minX), minY(minY), maxX(maxX), maxY(maxY)
    {
    }

    /**
     * @brief Sets the boundary limits.
     * 
     * @param newMinX Minimum X coordinate allowed.
     * @param newMinY Minimum Y coordinate allowed.
     * @param newMaxX Maximum X coordinate allowed.
     * @param newMaxY Maximum Y coordinate allowed.
     */
    void setBoundaries(float newMinX, float newMinY, float newMaxX, float newMaxY)
    {
        minX = newMinX;
        minY = newMinY;
        maxX = newMaxX;
        maxY = newMaxY;
    }

    /**
     * @brief Checks if a given coordinate point is within the boundaries.
     * 
     * @param x The X coordinate to check.
     * @param y The Y coordinate to check.
     * @return true If the coordinate is within the boundaries.
     * @return false If the coordinate is outside the boundaries.
     */
    [[nodiscard]] bool isWithinBoundaries(float x, float y) const
    {
        return (x >= minX && x <= maxX && y >= minY && y <= maxY);
    }

    /**
     * @brief Gets the current minimum X boundary.
     * 
     * @return float The minimum X boundary.
     */
    [[nodiscard]] float getMinX() const { return minX; }

    /**
     * @brief Gets the current minimum Y boundary.
     * 
     * @return float The minimum Y boundary.
     */
    [[nodiscard]] float getMinY() const { return minY; }

    /**
     * @brief Gets the current maximum X boundary.
     * 
     * @return float The maximum X boundary.
     */
    [[nodiscard]] float getMaxX() const { return maxX; }

    /**
     * @brief Gets the current maximum Y boundary.
     * 
     * @return float The maximum Y boundary.
     */
    [[nodiscard]] float getMaxY() const { return maxY; }

private:
    float minX{0.0f}; ///< Minimum allowed X coordinate.
    float minY{0.0f}; ///< Minimum allowed Y coordinate.
    float maxX{0.0f}; ///< Maximum allowed X coordinate.
    float maxY{0.0f}; ///< Maximum allowed Y coordinate.
};
