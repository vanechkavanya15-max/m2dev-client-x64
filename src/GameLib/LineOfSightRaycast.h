#pragma once

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <concepts>

/**
 * @brief Class providing line of sight checking capabilities using Bresenham's algorithm.
 * 
 * This class abstracts the 2D raycasting logic, completely decoupled from the GUI 
 * and specific map data structures. It strictly operates on memory state and uses 
 * modern C++20 concepts to accept generic obstacle-checking predicates.
 */
class LineOfSightRaycast
{
public:
    /**
     * @brief Performs a 2D line of sight check from a start point to an end point.
     * 
     * Iterates over points forming a line between (startX, startY) and (endX, endY)
     * using Bresenham's line algorithm. At each step, it evaluates the `isObstacle` predicate.
     * The evaluation is fully determined by the caller's logic.
     * 
     * @tparam Predicate A callable type taking two int32_t arguments (x, y) and returning a boolean-testable value.
     * @param startX Starting X coordinate of the raycast.
     * @param startY Starting Y coordinate of the raycast.
     * @param endX Ending X coordinate of the raycast.
     * @param endY Ending Y coordinate of the raycast.
     * @param isObstacle Predicate to check if a specific grid cell contains an obstacle.
     * @return true If the path is clear (no obstacles hit).
     * @return false If the path is blocked by an obstacle.
     */
    template <typename Predicate>
    requires std::predicate<Predicate, int32_t, int32_t>
    static bool CheckLineOfSight(int32_t startX, int32_t startY, int32_t endX, int32_t endY, Predicate isObstacle)
    {
        int32_t currentX = startX;
        int32_t currentY = startY;

        int32_t deltaX = std::abs(endX - currentX);
        int32_t deltaY = std::abs(endY - currentY);

        int32_t stepX = (currentX < endX) ? 1 : -1;
        int32_t stepY = (currentY < endY) ? 1 : -1;

        int32_t error = deltaX - deltaY;

        while (true)
        {
            if (isObstacle(currentX, currentY))
            {
                return false;
            }

            if (currentX == endX && currentY == endY)
            {
                break;
            }

            int32_t doubleError = 2 * error;

            if (doubleError > -deltaY)
            {
                error -= deltaY;
                currentX += stepX;
            }

            if (doubleError < deltaX)
            {
                error += deltaX;
                currentY += stepY;
            }
        }

        return true;
    }
};
