#pragma once

#include <cstdint>
#include <optional>
#include <cmath>

namespace Navigation {

/**
 * @brief Represents coordinates sent from/to the server, typically in centimeters.
 */
struct ServerCoord {
    int32_t x{0};
    int32_t y{0};
};

/**
 * @brief Represents 3D/2D pixel coordinates used in the local game world.
 */
struct PixelCoord {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

/**
 * @brief Represents grid tile (cell) coordinates, used for terrain and objects.
 */
struct TileCoord {
    int32_t x{0};
    int32_t y{0};
};

/**
 * @brief Represents smaller attribute tile (cell) coordinates, used for pathfinding and collisions.
 */
struct AttributeTileCoord {
    int32_t x{0};
    int32_t y{0};
};

/**
 * @brief Utility class for coordinate transformations.
 * 
 * Provides safe conversions between server units, pixel units, and grid tiles.
 * Evaluates boundary conditions and floats for NaN/Infinity.
 */
class CoordTransformer {
public:
    static constexpr int32_t TILE_SIZE = 100;
    static constexpr int32_t ATTRIBUTE_TILE_SIZE = 50;

    /**
     * @brief Converts server coordinates (integer) to pixel coordinates (float).
     * @param serverCoord The server coordinates.
     * @return std::optional<PixelCoord> Resulting pixel coordinates, or std::nullopt on error.
     */
    static std::optional<PixelCoord> ServerToPixel(const ServerCoord& serverCoord) {
        return PixelCoord{static_cast<float>(serverCoord.x), static_cast<float>(serverCoord.y), 0.0f};
    }

    /**
     * @brief Converts pixel coordinates to server coordinates.
     * @param pixelCoord The pixel coordinates.
     * @return std::optional<ServerCoord> Resulting server coordinates, or std::nullopt if pixel coords are invalid.
     */
    static std::optional<ServerCoord> PixelToServer(const PixelCoord& pixelCoord) {
        if (std::isnan(pixelCoord.x) || std::isinf(pixelCoord.x) ||
            std::isnan(pixelCoord.y) || std::isinf(pixelCoord.y)) {
            return std::nullopt;
        }

        // Check for bounds before casting to int32_t to prevent undefined behavior
        constexpr float maxInt = static_cast<float>(INT32_MAX);
        constexpr float minInt = static_cast<float>(INT32_MIN);

        if (pixelCoord.x > maxInt || pixelCoord.x < minInt ||
            pixelCoord.y > maxInt || pixelCoord.y < minInt) {
            return std::nullopt;
        }

        return ServerCoord{static_cast<int32_t>(pixelCoord.x), static_cast<int32_t>(pixelCoord.y)};
    }

    /**
     * @brief Converts pixel coordinates to tile coordinates.
     * @param pixelCoord The pixel coordinates.
     * @return std::optional<TileCoord> Resulting tile coordinates, or std::nullopt if pixel coords are invalid.
     */
    static std::optional<TileCoord> PixelToTile(const PixelCoord& pixelCoord) {
        if (std::isnan(pixelCoord.x) || std::isinf(pixelCoord.x) ||
            std::isnan(pixelCoord.y) || std::isinf(pixelCoord.y)) {
            return std::nullopt;
        }

        float tileX = pixelCoord.x / static_cast<float>(TILE_SIZE);
        float tileY = pixelCoord.y / static_cast<float>(TILE_SIZE);

        constexpr float maxInt = static_cast<float>(INT32_MAX);
        constexpr float minInt = static_cast<float>(INT32_MIN);

        if (tileX > maxInt || tileX < minInt || tileY > maxInt || tileY < minInt) {
            return std::nullopt;
        }

        return TileCoord{static_cast<int32_t>(tileX), static_cast<int32_t>(tileY)};
    }

    /**
     * @brief Converts tile coordinates to pixel coordinates.
     * @param tileCoord The tile coordinates.
     * @return std::optional<PixelCoord> Resulting pixel coordinates.
     */
    static std::optional<PixelCoord> TileToPixel(const TileCoord& tileCoord) {
        float x = static_cast<float>(tileCoord.x) * static_cast<float>(TILE_SIZE);
        float y = static_cast<float>(tileCoord.y) * static_cast<float>(TILE_SIZE);

        if (std::isinf(x) || std::isinf(y)) {
            return std::nullopt;
        }

        return PixelCoord{x, y, 0.0f};
    }

    /**
     * @brief Converts pixel coordinates to attribute tile coordinates.
     * @param pixelCoord The pixel coordinates.
     * @return std::optional<AttributeTileCoord> Resulting attribute tile coordinates, or std::nullopt if invalid.
     */
    static std::optional<AttributeTileCoord> PixelToAttributeTile(const PixelCoord& pixelCoord) {
        if (std::isnan(pixelCoord.x) || std::isinf(pixelCoord.x) ||
            std::isnan(pixelCoord.y) || std::isinf(pixelCoord.y)) {
            return std::nullopt;
        }

        float attrX = pixelCoord.x / static_cast<float>(ATTRIBUTE_TILE_SIZE);
        float attrY = pixelCoord.y / static_cast<float>(ATTRIBUTE_TILE_SIZE);

        constexpr float maxInt = static_cast<float>(INT32_MAX);
        constexpr float minInt = static_cast<float>(INT32_MIN);

        if (attrX > maxInt || attrX < minInt || attrY > maxInt || attrY < minInt) {
            return std::nullopt;
        }

        return AttributeTileCoord{static_cast<int32_t>(attrX), static_cast<int32_t>(attrY)};
    }

    /**
     * @brief Converts attribute tile coordinates to pixel coordinates.
     * @param attrTileCoord The attribute tile coordinates.
     * @return std::optional<PixelCoord> Resulting pixel coordinates.
     */
    static std::optional<PixelCoord> AttributeTileToPixel(const AttributeTileCoord& attrTileCoord) {
        float x = static_cast<float>(attrTileCoord.x) * static_cast<float>(ATTRIBUTE_TILE_SIZE);
        float y = static_cast<float>(attrTileCoord.y) * static_cast<float>(ATTRIBUTE_TILE_SIZE);

        if (std::isinf(x) || std::isinf(y)) {
            return std::nullopt;
        }

        return PixelCoord{x, y, 0.0f};
    }

    /**
     * @brief Converts server coordinates to tile coordinates.
     * @param serverCoord The server coordinates.
     * @return std::optional<TileCoord> Resulting tile coordinates.
     */
    static std::optional<TileCoord> ServerToTile(const ServerCoord& serverCoord) {
        auto pixelCoord = ServerToPixel(serverCoord);
        if (!pixelCoord) {
            return std::nullopt;
        }
        return PixelToTile(*pixelCoord);
    }
    
    /**
     * @brief Converts server coordinates to attribute tile coordinates.
     * @param serverCoord The server coordinates.
     * @return std::optional<AttributeTileCoord> Resulting attribute tile coordinates.
     */
    static std::optional<AttributeTileCoord> ServerToAttributeTile(const ServerCoord& serverCoord) {
        auto pixelCoord = ServerToPixel(serverCoord);
        if (!pixelCoord) {
            return std::nullopt;
        }
        return PixelToAttributeTile(*pixelCoord);
    }
};

} // namespace Navigation
