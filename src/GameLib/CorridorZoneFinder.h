#pragma once

#include <vector>
#include <optional>
#include <expected>
#include <cstdint>
#include <format>
#include <ranges>
#include <span>
#include <algorithm>
#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event emitted when a bottleneck or corridor is detected.
 */
struct CorridorDetectedEvent : public Core::IEvent {
    EterBase::MapIndex mapIndex;
    int32_t x;
    int32_t y;
    int32_t width;

    /**
     * @brief Constructs the event.
     * @param mapIndex Map index where the corridor was found.
     * @param x X coordinate of the center of the corridor.
     * @param y Y coordinate of the center of the corridor.
     * @param width Width of the corridor.
     */
    CorridorDetectedEvent(EterBase::MapIndex mapIndex, int32_t x, int32_t y, int32_t width)
        : mapIndex(mapIndex), x(x), y(y), width(width) {}
};

/**
 * @brief Represents a coordinate in 2D space.
 */
struct Coordinate {
    int32_t x;
    int32_t y;
};

/**
 * @brief Logic for finding bottlenecks/corridors on a map grid.
 */
class CorridorZoneFinder {
public:
    /**
     * @brief Result of a corridor search operation.
     */
    struct SearchResult {
        std::vector<Coordinate> bottlenecks;
    };

    /**
     * @brief Default constructor.
     */
    CorridorZoneFinder() = default;

    /**
     * @brief Analyzes a 2D grid to detect horizontal and vertical bottlenecks (corridors).
     * 
     * @param mapIndex Index of the map being searched.
     * @param grid A flattened 2D span representing grid cells (1 for obstacle, 0 for empty space).
     * @param width The width of the grid.
     * @param height The height of the grid.
     * @param maxCorridorWidth The maximum width in cells to be considered a corridor/bottleneck.
     * @return std::expected<SearchResult, EterBase::NavigationError>
     */
    std::expected<SearchResult, EterBase::NavigationError> FindCorridors(
        EterBase::MapIndex mapIndex, 
        std::span<const uint8_t> grid, 
        int32_t width, 
        int32_t height, 
        int32_t maxCorridorWidth) const 
    {
        if (width <= 0 || height <= 0 || grid.size() != static_cast<size_t>(width * height)) {
            EterBase::ModernLogger::Error("Invalid grid dimensions: {}x{}, size {}", width, height, grid.size());
            return std::unexpected(EterBase::NavigationError::MapNotLoaded);
        }

        if (maxCorridorWidth <= 0) {
            return std::unexpected(EterBase::NavigationError::PathNotFound);
        }

        EterBase::ModernLogger::Info("Searching for corridors in map {} ({}x{})", mapIndex.value(), width, height);

        SearchResult result;
        
        for (int32_t y = 1; y < height - 1; ++y) {
            int32_t skipX = 0;
            for (int32_t x = 1; x < width - 1; ++x) {
                if (skipX > 0) {
                    skipX--;
                    continue;
                }
                
                if (grid[y * width + x] == 0) {
                    auto processBottleneck = [&](const Coordinate& coord) -> Coordinate {
                        result.bottlenecks.push_back(coord);
                        Core::EventBus::Instance().Publish(CorridorDetectedEvent{mapIndex, coord.x, coord.y, maxCorridorWidth});
                        EterBase::ModernLogger::Info("Emitted CorridorDetectedEvent at ({}, {})", coord.x, coord.y);
                        return coord;
                    };

                    std::optional<Coordinate> found = CheckHorizontalBottleneck(grid, width, height, x, y, maxCorridorWidth)
                        .transform(processBottleneck)
                        .or_else([&]() {
                            return CheckVerticalBottleneck(grid, width, height, x, y, maxCorridorWidth)
                                .transform(processBottleneck);
                        });
                        
                    found.transform([&](const Coordinate&) {
                        skipX = maxCorridorWidth; // Skip ahead horizontally to avoid duplicate detections
                        return true;
                    });
                }
            }
        }

        return result;
    }

private:
    /**
     * @brief Checks if a given empty cell is part of a horizontal bottleneck.
     */
    std::optional<Coordinate> CheckHorizontalBottleneck(
        std::span<const uint8_t> grid, 
        int32_t width, 
        int32_t /*height*/, 
        int32_t x, 
        int32_t y, 
        int32_t maxCorridorWidth) const 
    {
        int32_t leftDist = 0;
        while (x - leftDist >= 0 && grid[y * width + (x - leftDist)] == 0) {
            leftDist++;
        }
        
        int32_t rightDist = 0;
        while (x + rightDist < width && grid[y * width + (x + rightDist)] == 0) {
            rightDist++;
        }

        int32_t totalWidth = leftDist + rightDist - 1;
        if (totalWidth > 0 && totalWidth <= maxCorridorWidth) {
            int32_t centerX = x - leftDist + 1 + (totalWidth / 2);
            return Coordinate{centerX, y};
        }
        
        return std::nullopt;
    }

    /**
     * @brief Checks if a given empty cell is part of a vertical bottleneck.
     */
    std::optional<Coordinate> CheckVerticalBottleneck(
        std::span<const uint8_t> grid, 
        int32_t width, 
        int32_t height, 
        int32_t x, 
        int32_t y, 
        int32_t maxCorridorWidth) const 
    {
        int32_t upDist = 0;
        while (y - upDist >= 0 && grid[(y - upDist) * width + x] == 0) {
            upDist++;
        }
        
        int32_t downDist = 0;
        while (y + downDist < height && grid[(y + downDist) * width + x] == 0) {
            downDist++;
        }

        int32_t totalHeight = upDist + downDist - 1;
        if (totalHeight > 0 && totalHeight <= maxCorridorWidth) {
            int32_t centerY = y - upDist + 1 + (totalHeight / 2);
            return Coordinate{x, centerY};
        }
        
        return std::nullopt;
    }
};

} // namespace GameLib
