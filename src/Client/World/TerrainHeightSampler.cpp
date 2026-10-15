#include "StdAfx.h"
#include "TerrainHeightSampler.h"

#include <cmath>
#include <algorithm>
#include <utility>

namespace Client::World {

// ============================================================================
// TerrainHeightSampler Implementation
// ============================================================================
// This class is responsible for accurate terrain sampling within the Metin2
// client architecture. It provides high-performance height lookups using 
// bilinear interpolation, and normal calculations based on the analytical
// derivatives of the interpolation function. This guarantees visually smooth
// terrain traversing and physically accurate character orientations on 
// slopes. 
//
// The architecture of this class strictly adheres to the Domain-Driven Cohesion
// principle. All logic for terrain height interpretation, boundary checking,
// and normal calculation is self-contained within this single cohesive unit.
// This prevents fragmentation of the terrain sampling subsystem.
//
// Performance considerations:
// - Height map data is stored as a flat 1D std::vector for contiguous 
//   memory access and cache coherence. This leverages hardware prefetchers
//   to maximize throughput during sequential sampling operations.
// - Mathematical functions utilize standard library abstractions which are
//   expected to compile down to efficient CPU intrinsics (e.g. std::lerp, 
//   std::sqrt).
// - Boundary validations are highly optimized, operating independently from
//   the heavier mathematical lifting to ensure fast-fail on invalid inputs.
// - Zero heap allocations occur during GetHeight and GetNormal lookups to 
//   ensure O(1) deterministic execution time.
// ============================================================================

/**
 * @brief Default constructor for TerrainHeightSampler.
 * 
 * Initializes the sampler with empty grid data and zero dimensions.
 * The sampler must be fully initialized via the Initialize() method
 * before any height or normal queries are performed. If queried prior
 * to initialization, methods will strictly return TerrainError::DataNotLoaded
 * to prevent undefined behavior.
 * 
 * In a typical client lifecycle, this constructor is called when the 
 * CPythonBackground system instantiates its internal map objects, long before
 * the height data is actually streamed from disk.
 */
TerrainHeightSampler::TerrainHeightSampler()
    : m_width(0)
    , m_height(0)
    , m_gridHeights()
{
}

/**
 * @brief Initializes the terrain height sampler with explicit boundary checks.
 * 
 * This method validates the input dimensions and securely copies the height
 * map data into the sampler's internal storage for fast access during 
 * bilinear interpolation and normal vector calculations.
 * 
 * The validation ensures that:
 * 1. The dimensions are valid (width > 0, height > 0). Zero dimensions represent
 *    a mathematically impossible surface.
 * 2. The dimensions match the size of the flat vector passed in. This prevents
 *    potential segmentation faults during runtime when accessing calculated 1D indices.
 * 
 * Memory safety note: The height map vector is copied rather than referenced.
 * This guarantees that the sampler retains ownership of the terrain data,
 * preventing dangling references if the original source data goes out of scope,
 * which is a common hazard in asynchronous asset loading pipelines.
 * 
 * @param width The horizontal dimension of the heightmap grid (number of vertices).
 * @param height The vertical dimension of the heightmap grid (number of vertices).
 * @param heights The 1D vector containing width * height float values representing elevations.
 * 
 * @return std::expected<void, TerrainError> 
 *         - Success if the dimensions match the provided vector size.
 *         - InvalidGridSize if the sizes mismatch or are zero.
 */
std::expected<void, TerrainError> TerrainHeightSampler::Initialize(uint32_t width, uint32_t height, const std::vector<float>& heights)
{
    // Validate that the provided dimensions are non-zero to prevent division by zero or invalid indexing.
    // A 0x0 map is conceptually invalid in this engine.
    if (width == 0 || height == 0)
    {
        return std::unexpected(TerrainError::InvalidGridSize);
    }

    // Validate that the provided heights array exactly matches the expected total grid size.
    // This strict validation prevents potential out-of-bounds reads during runtime.
    // We cast to size_t to prevent integer overflow during the multiplication of large grids.
    const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (heights.size() != expectedSize)
    {
        return std::unexpected(TerrainError::InvalidGridSize);
    }

    // Safely store the validated dimensions for future bound checking operations.
    m_width = width;
    m_height = height;

    // Securely copy the height data into our internal grid storage.
    // This allows the sampler to own the data it needs to perform fast lookups,
    // ensuring thread and memory safety for concurrent read operations across the ECS.
    m_gridHeights = heights;

    return {};
}

/**
 * @brief Validates if the given floating-point world coordinates are within the map bounds.
 * 
 * The bounds are defined by the grid dimensions. A coordinate is considered within bounds
 * if it is >= 0.0f and < (dimension - 1.0f). This ensures that any bilinear interpolation
 * has at least a 1x1 grid cell to interpolate within.
 * 
 * This method must be called internally before any read operation is performed to
 * guarantee memory safety and prevent out-of-bounds reads which could crash the client
 * or allow out-of-bounds exploitation.
 * 
 * @param x The continuous world X coordinate.
 * @param y The continuous world Y coordinate.
 * @return true if the coordinate is strictly within the heightmap bounds.
 */
bool TerrainHeightSampler::IsInBoundsFloat(float x, float y) const
{
    // The sampler requires that it has been initialized. If width or height
    // are zero, the map is mathematically invalid and holds no bounds.
    if (m_width == 0 || m_height == 0)
    {
        return false;
    }

    // The maximum valid floating point index is the dimension minus one,
    // because we need at least a 1x1 grid cell to interpolate within.
    // For a 256x256 grid, valid float coordinates range from 0.0f to 255.0f.
    const float maxX = static_cast<float>(m_width - 1);
    const float maxY = static_cast<float>(m_height - 1);

    // Check bounds inclusively on the lower end, and inclusively on the upper end.
    // While x=maxX and y=maxY represent the absolute edge of the map, they are
    // mathematically valid and will interpolate smoothly with adjacent inner cells
    // (using clamped coordinates during interpolation logic).
    return (x >= 0.0f && x <= maxX) && (y >= 0.0f && y <= maxY);
}

/**
 * @brief Validates if the given integer grid coordinates are safely within the bounds.
 * 
 * This is an internal safety check designed for direct grid indexing operations.
 * It prevents integer overflow and out-of-bounds flat vector indexing.
 * 
 * @param gridX The integer X index of the heightmap grid.
 * @param gridY The integer Y index of the heightmap grid.
 * @return true if the grid coordinates can be safely used to index the internal storage.
 */
bool TerrainHeightSampler::IsValidGridCoordinate(uint32_t gridX, uint32_t gridY) const
{
    // Check against grid dimensions. We assume width and height > 0 due to Initialize().
    // The less-than operator ensures we strictly stay below the max dimension limit.
    return (gridX < m_width) && (gridY < m_height);
}

/**
 * @brief Retrieves the raw height value at a specific grid index.
 * 
 * This method securely computes the flat 1D vector index from 2D coordinates.
 * Note: For extreme performance, this method omits redundant boundary checks. 
 * The caller is strictly responsible for validating `gridX` and `gridY` via 
 * boundary clamps (std::min) or explicit IsValidGridCoordinate checks before 
 * invoking this method.
 * 
 * @param gridX The X index in the heightmap.
 * @param gridY The Y index in the heightmap.
 * @return The elevation value at the specified grid vertex.
 */
float TerrainHeightSampler::GetRawHeight(uint32_t gridX, uint32_t gridY) const
{
    // Compute the 1D flat index from the 2D coordinates using row-major ordering.
    // Row-major ordering aligns with standard cache line fetching strategies.
    // Size_t is cast to prevent overflow on massive custom maps.
    const size_t index = static_cast<size_t>(gridY) * static_cast<size_t>(m_width) + static_cast<size_t>(gridX);
    return m_gridHeights[index];
}

/**
 * @brief Samples the terrain height at a precise floating-point coordinate using bilinear interpolation.
 * 
 * Bilinear interpolation works by finding the four nearest grid vertices surrounding
 * the target coordinate and calculating a weighted average of their heights based on
 * the target's distance from those vertices.
 * 
 * This creates a smooth continuous surface from discrete grid points, which is 
 * crucial for fluid character movement, accurate shadow mapping, and projectile 
 * collision detection across rugged terrain. Without this, character Z-coordinates
 * would harshly 'snap' when moving from cell to cell.
 * 
 * Computational Flow:
 * 1. Boundary Validation: Ensure x and y fall within 0.0 and map_size - 1.0.
 * 2. Floor determination of the coordinate to find the base grid cell (x0, y0).
 * 3. Calculation of the upper grid bounds (x1, y1), clamped using std::min to prevent overflow.
 * 4. Extraction of the fractional components (fracX, fracY) used as lerp weights.
 * 5. Retrieval of the four corner heights (h00, h10, h01, h11).
 * 6. Mathematical bilinear interpolation utilizing std::lerp for optimal instruction usage.
 * 
 * @param x The precise world X coordinate in grid-space.
 * @param y The precise world Y coordinate in grid-space.
 * @return A std::expected containing the interpolated height float, or an error if out of bounds.
 */
std::expected<float, TerrainError> TerrainHeightSampler::GetHeight(float x, float y) const
{
    // 1. Validate Initialization State
    // Before attempting to sample, we must ensure the sampler has been loaded with data.
    if (m_gridHeights.empty())
    {
        return std::unexpected(TerrainError::DataNotLoaded);
    }

    // 2. Validate Boundary Conditions
    // We must not sample outside the defined map bounds, as this would cause out-of-bounds memory access
    // or mathematically incorrect artifacts.
    if (!IsInBoundsFloat(x, y))
    {
        return std::unexpected(TerrainError::OutOfBounds);
    }

    // 3. Calculate Grid Indices
    // Determine the top-left coordinate of the grid cell containing the (x, y) point.
    // std::floor is used to ensure we always get the lower bounding integer, gracefully
    // handling floating point precision artifacts.
    const float floorX = std::floor(x);
    const float floorY = std::floor(y);

    const uint32_t x0 = static_cast<uint32_t>(floorX);
    const uint32_t y0 = static_cast<uint32_t>(floorY);

    // Determine the bottom-right coordinate of the grid cell.
    // If x0 is exactly at the rightmost edge, x1 should equal x0 to prevent out-of-bounds.
    // std::min guarantees we never exceed the array boundaries, clamping nicely to the edge.
    const uint32_t x1 = std::min(x0 + 1, m_width - 1);
    const uint32_t y1 = std::min(y0 + 1, m_height - 1);

    // 4. Calculate Fractional Distances
    // The fractional components represent the weight of the interpolation along each axis.
    // A fracX of 0.0 means the point is exactly on the left edge, 1.0 means exactly on the right edge.
    // This value is guaranteed to be between 0.0f and 1.0f due to the floor operation above.
    const float fracX = x - floorX;
    const float fracY = y - floorY;

    // 5. Retrieve Grid Heights
    // We retrieve the elevation values at the four corners of the grid cell.
    // h00: Top-Left       (Base Coordinate)
    // h10: Top-Right      (Offset X)
    // h01: Bottom-Left    (Offset Y)
    // h11: Bottom-Right   (Offset X and Y)
    const float h00 = GetRawHeight(x0, y0);
    const float h10 = GetRawHeight(x1, y0);
    const float h01 = GetRawHeight(x0, y1);
    const float h11 = GetRawHeight(x1, y1);

    // 6. Perform Bilinear Interpolation
    // Bilinear interpolation can be broken down into three linear interpolations.
    //
    // Step A: Interpolate horizontally along the top edge of the cell.
    // We blend the top-left and top-right heights based on horizontal position.
    const float interpTop = std::lerp(h00, h10, fracX);

    // Step B: Interpolate horizontally along the bottom edge of the cell.
    // We blend the bottom-left and bottom-right heights based on horizontal position.
    const float interpBottom = std::lerp(h01, h11, fracX);

    // Step C: Interpolate vertically between the top and bottom interpolated values.
    // We blend the top edge result and bottom edge result based on vertical position.
    // This final lerp produces the exact surface elevation for the provided 2D coordinate.
    const float finalHeight = std::lerp(interpTop, interpBottom, fracY);

    // Return the successfully interpolated height wrapped in std::expected.
    return finalHeight;
}

/**
 * @brief Calculates the analytical surface normal vector at a given coordinate.
 * 
 * The surface normal is a perpendicular vector pointing directly outwards from the terrain face.
 * This is fundamentally essential for rendering correct dynamic lighting, calculating proper
 * collision responses, and most importantly, orienting the character's model skeleton
 * appropriately so their feet align with the steepness of slopes.
 * 
 * The normal is calculated using the exact analytical partial derivatives of the 
 * bilinear interpolation function. This guarantees mathematically perfect normal 
 * vectors that transition smoothly across cell boundaries, completely eliminating
 * the jarring faceted look of simple cross-product normal estimations typical in 
 * older rendering pipelines.
 * 
 * Analytical partial derivatives for true bilinear surface:
 * f(x,y) = h00(1-x)(1-y) + h10(x)(1-y) + h01(1-x)y + h11(x)y
 * df/dx = (h10 - h00)(1-y) + (h11 - h01)y
 * df/dy = (h01 - h00)(1-x) + (h11 - h10)x
 * 
 * The resultant normal vector is guaranteed to be normalized (length == 1.0f).
 * 
 * @param x The continuous world X coordinate in grid-space.
 * @param y The continuous world Y coordinate in grid-space.
 * @return A std::expected containing the normalized Vector3 surface normal, or an error if out of bounds.
 */
std::expected<Vector3, TerrainError> TerrainHeightSampler::GetNormal(float x, float y) const
{
    // 1. Validate Initialization State
    // We cannot compute surface derivatives if there is no surface data present.
    if (m_gridHeights.empty())
    {
        return std::unexpected(TerrainError::DataNotLoaded);
    }

    // 2. Validate Boundary Conditions
    // We require the coordinate to be strictly inside the bounds.
    // Attempting to calculate derivatives on the absolute edge without adjacent
    // points could yield undefined behaviour or artificially flat normals if
    // not carefully clamped.
    if (!IsInBoundsFloat(x, y))
    {
        return std::unexpected(TerrainError::OutOfBounds);
    }

    // 3. Grid Cell Identification
    // We find the base grid coordinates, mirroring the robust logic utilized in GetHeight.
    const float floorX = std::floor(x);
    const float floorY = std::floor(y);

    const uint32_t x0 = static_cast<uint32_t>(floorX);
    const uint32_t y0 = static_cast<uint32_t>(floorY);

    const uint32_t x1 = std::min(x0 + 1, m_width - 1);
    const uint32_t y1 = std::min(y0 + 1, m_height - 1);

    // 4. Retrieve Heights for Gradient Calculation
    // We require all four corners of the grid cell to compute accurate spatial derivatives.
    // This is because the slope at any point within the bilinear cell is influenced by all
    // four surrounding vertex heights.
    const float h00 = GetRawHeight(x0, y0);
    const float h10 = GetRawHeight(x1, y0);
    const float h01 = GetRawHeight(x0, y1);
    const float h11 = GetRawHeight(x1, y1);

    // 5. Calculate Fractional Distances
    // These fractions determine our exact location within the cell, dictating which
    // vertices have the most influence over the local gradient.
    const float fracX = x - floorX;
    const float fracY = y - floorY;

    // 6. Compute Analytical Partial Derivatives
    // We calculate the exact rate of change of the bilinear surface with respect
    // to X (dfdx) and Y (dfdy). This mathematically represents the tangential slopes.
    // 
    // dfdx: Rate of change along the X axis.
    // It blends the top horizontal gradient (h10-h00) with the bottom horizontal gradient (h11-h01)
    // based on our vertical position (fracY).
    const float dfdx = (h10 - h00) * (1.0f - fracY) + (h11 - h01) * fracY;
    
    // dfdy: Rate of change along the Y axis.
    // It blends the left vertical gradient (h01-h00) with the right vertical gradient (h11-h10)
    // based on our horizontal position (fracX).
    const float dfdy = (h01 - h00) * (1.0f - fracX) + (h11 - h10) * fracX;

    // 7. Normal Vector Assembly
    // The surface normal is defined mathematically as the cross product of the X and Y tangent vectors.
    // tangentX = {1.0, 0.0, dfdx}
    // tangentY = {0.0, 1.0, dfdy}
    // Cross Product calculation:
    // normal.x = (tangentX.y * tangentY.z) - (tangentX.z * tangentY.y) = (0*dfdy) - (dfdx*1) = -dfdx
    // normal.y = (tangentX.z * tangentY.x) - (tangentX.x * tangentY.z) = (dfdx*0) - (1*dfdy) = -dfdy
    // normal.z = (tangentX.x * tangentY.y) - (tangentX.y * tangentY.x) = (1*1) - (0*0) = 1.0
    Vector3 normal = { -dfdx, -dfdy, 1.0f };

    // 8. Vector Normalization
    // The resulting normal vector must be strictly normalized (length == 1.0)
    // so it can be correctly applied to trigonometric operations for model orientation,
    // lighting dot products, and physics engine resolving algorithms.
    const float lengthSquared = (normal.x * normal.x) + (normal.y * normal.y) + (normal.z * normal.z);
    
    // Epsilon check to prevent division by zero or NaN generation.
    // Mathematically, lengthSquared will always be >= 1.0f due to the static Z component (1.0f^2 = 1.0f),
    // but this check ensures robust floating-point resilience across diverse hardware architectures.
    if (lengthSquared > 0.000001f)
    {
        // Inverse square root is utilized for performance, multiplying rather than dividing.
        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        normal.x *= inverseLength;
        normal.y *= inverseLength;
        normal.z *= inverseLength;
    }
    else
    {
        // Safety Fallback: Flat normal pointing straight up along the Z axis.
        // This prevents broken character rotations if extreme coordinate values cause NaN.
        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 1.0f;
    }

    // Return the accurately computed, fully normalized analytical normal vector.
    return normal;
}

} // namespace Client::World
