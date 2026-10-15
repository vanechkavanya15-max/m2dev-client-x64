#pragma once

#include <cstdint>
#include <span>
#include <optional>
#include <cmath>

namespace GameLib {

/**
 * @brief Provides lightweight bilinear interpolation for terrain height sampling.
 * 
 * This class encapsulates a raw 16-bit heightmap buffer and grid parameters,
 * allowing for efficient sampling of terrain height at arbitrary world coordinates.
 * It strictly separates the logic from GUI and other components.
 */
class HeightSampler {
public:
    /**
     * @brief Constructs a new HeightSampler instance.
     * 
     * @param heightmap Buffer containing 16-bit height values.
     * @param width Width of the heightmap grid in cells.
     * @param height Height of the heightmap grid in cells.
     * @param cellSize The physical world size of a single cell.
     */
    HeightSampler(std::span<const uint16_t> heightmap, uint32_t width, uint32_t height, float cellSize)
        : heightmap_(heightmap), width_(width), height_(height), cellSize_(cellSize) {}

    /**
     * @brief Samples the interpolated terrain height at given world coordinates.
     * 
     * @param worldX The world X coordinate.
     * @param worldY The world Y coordinate.
     * @return std::optional<float> The sampled height if within bounds and valid, std::nullopt otherwise.
     */
    std::optional<float> SampleHeight(float worldX, float worldY) const {
        if (heightmap_.empty() || cellSize_ <= 0.0f || width_ == 0 || height_ == 0) {
            return std::nullopt;
        }

        if (heightmap_.size() < static_cast<size_t>(width_) * height_) {
            return std::nullopt;
        }

        // Convert world coordinates to grid coordinates
        float gridX = worldX / cellSize_;
        float gridY = worldY / cellSize_;

        // Check bounds
        if (gridX < 0.0f || gridY < 0.0f || 
            gridX >= static_cast<float>(width_ - 1) || 
            gridY >= static_cast<float>(height_ - 1)) {
            return std::nullopt;
        }

        // Calculate fractional parts
        float xFloor = std::floor(gridX);
        float yFloor = std::floor(gridY);
        float xFraction = gridX - xFloor;
        float yFraction = gridY - yFloor;

        // Get indices for the 4 corners
        uint32_t x0 = static_cast<uint32_t>(xFloor);
        uint32_t y0 = static_cast<uint32_t>(yFloor);
        uint32_t x1 = x0 + 1;
        uint32_t y1 = y0 + 1;

        // Ensure we don't access out of bounds in case of float precision issues
        if (x1 >= width_ || y1 >= height_) {
            return std::nullopt;
        }

        // Get height values at corners
        float h00 = static_cast<float>(heightmap_[y0 * width_ + x0]);
        float h10 = static_cast<float>(heightmap_[y0 * width_ + x1]);
        float h01 = static_cast<float>(heightmap_[y1 * width_ + x0]);
        float h11 = static_cast<float>(heightmap_[y1 * width_ + x1]);

        // Bilinear interpolation
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
