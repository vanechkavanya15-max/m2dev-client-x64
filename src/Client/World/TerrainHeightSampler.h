#pragma once

#include <cstdint>
#include <vector>
#include <expected>

namespace Client::World {

    // Represents a 3D vector for normals and positions
    struct Vector3 {
        float x;
        float y;
        float z;
    };

    // Terrain error enumeration for std::expected
    enum class TerrainError {
        OutOfBounds,
        InvalidGridSize,
        DataNotLoaded
    };

    // Sampler class responsible for terrain height and normal calculations
    class TerrainHeightSampler {
    public:
        TerrainHeightSampler();
        ~TerrainHeightSampler() = default;

        /**
         * Initialize the terrain sampler with grid dimensions and height data.
         * @param width The number of vertices along the X axis.
         * @param height The number of vertices along the Y axis.
         * @param heights A flat vector of height values for each vertex.
         * @return std::expected<void, TerrainError> indicating success or failure.
         */
        std::expected<void, TerrainError> Initialize(uint32_t width, uint32_t height, const std::vector<float>& heights);

        /**
         * Samples the height at a given 2D position using bilinear interpolation.
         * @param x The world X coordinate.
         * @param y The world Y coordinate.
         * @return The interpolated height, or a TerrainError if out of bounds.
         */
        std::expected<float, TerrainError> GetHeight(float x, float y) const;

        /**
         * Calculates the surface normal at a given 2D position for character inclination on slopes.
         * @param x The world X coordinate.
         * @param y The world Y coordinate.
         * @return The computed normal Vector3, or a TerrainError if out of bounds.
         */
        std::expected<Vector3, TerrainError> GetNormal(float x, float y) const;

    private:
        /**
         * Safely validates if the given grid coordinates are within the map bounds.
         * @param gridX The X grid index.
         * @param gridY The Y grid index.
         * @return true if within bounds, false otherwise.
         */
        bool IsValidGridCoordinate(uint32_t gridX, uint32_t gridY) const;

        /**
         * Safely validates if the given world coordinates are within the map bounds.
         * @param x The world X coordinate.
         * @param y The world Y coordinate.
         * @return true if within bounds, false otherwise.
         */
        bool IsInBoundsFloat(float x, float y) const;

        /**
         * Retrieves the raw height value at specific grid coordinates without bounds checking.
         * Internal method only. Should only be called after IsValidGridCoordinate check.
         * @param gridX The X grid index.
         * @param gridY The Y grid index.
         * @return The raw height float.
         */
        float GetRawHeight(uint32_t gridX, uint32_t gridY) const;

        uint32_t m_width;
        uint32_t m_height;
        std::vector<float> m_gridHeights;
    };

} // namespace Client::World
