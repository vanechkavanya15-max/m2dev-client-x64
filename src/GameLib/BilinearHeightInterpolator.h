#pragma once

#include <cstdint>
#include <span>
#include <expected>
#include <cmath>
#include "../EterBase/Result.h"

namespace GameLib {

/**
 * @brief Bilinear interpolation for terrain height sampling.
 * 
 * This class is responsible for calculating terrain Z height using 
 * bilinear interpolation based on a raw heightmap. It strictly operates 
 * on the CPU and is independent of graphics libraries like DirectX.
 */
class BilinearHeightInterpolator {
public:
    /**
     * @brief Constructs a new BilinearHeightInterpolator instance.
     * 
     * @param heightmap Buffer containing 16-bit height values.
     * @param width Width of the heightmap grid in cells.
     * @param height Height of the heightmap grid in cells.
     * @param cellSize The physical world size of a single cell.
     */
    BilinearHeightInterpolator(std::span<const uint16_t> heightmap, uint32_t width, uint32_t height, float cellSize)
        : heightmap_(heightmap), width_(width), height_(height), cellSize_(cellSize) {}

    /**
     * @brief Samples the interpolated terrain height at given world coordinates.
     * 
     * @param worldX The world X coordinate.
     * @param worldY The world Y coordinate.
     * @return std::expected<float, EterBase::NavigationError> The interpolated height or a NavigationError on failure.
     */
    [[nodiscard]] std::expected<float, EterBase::NavigationError> GetHeight(float worldX, float worldY) const {
        if (heightmap_.empty() || cellSize_ <= 0.0f || width_ == 0 || height_ == 0) {
            return std::unexpected(EterBase::NavigationError::MapNotLoaded);
        }

        if (heightmap_.size() < static_cast<size_t>(width_) * height_) {
            return std::unexpected(EterBase::NavigationError::MapNotLoaded);
        }

        float gridX = worldX / cellSize_;
        float gridY = worldY / cellSize_;

        if (gridX < 0.0f || gridY < 0.0f || 
            gridX >= static_cast<float>(width_ - 1) || 
            gridY >= static_cast<float>(height_ - 1)) {
            return std::unexpected(EterBase::NavigationError::DestinationUnreachable);
        }

        float xFloor = std::floor(gridX);
        float yFloor = std::floor(gridY);
        float xFraction = gridX - xFloor;
        float yFraction = gridY - yFloor;

        uint32_t x0 = static_cast<uint32_t>(xFloor);
        uint32_t y0 = static_cast<uint32_t>(yFloor);
        uint32_t x1 = x0 + 1;
        uint32_t y1 = y0 + 1;

        // Using standard vector access (no multi-dimensional overload since it's a 1D span representing a grid)
        float h00 = static_cast<float>(heightmap_[y0 * width_ + x0]);
        float h10 = static_cast<float>(heightmap_[y0 * width_ + x1]);
        float h01 = static_cast<float>(heightmap_[y1 * width_ + x0]);
        float h11 = static_cast<float>(heightmap_[y1 * width_ + x1]);

        float h0 = h00 * (1.0f - xFraction) + h10 * xFraction;
        float h1 = h01 * (1.0f - xFraction) + h11 * xFraction;
        float result = h0 * (1.0f - yFraction) + h1 * yFraction;

        return result;
    }

private:
    std::span<const uint16_t> heightmap_;
    uint32_t width_;
    uint32_t height_;
    float cellSize_;
};

} // namespace GameLib
