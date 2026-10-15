#include "TerrainHeightSampler.h"
#include <algorithm>

namespace EterLib::Render {

void TerrainHeightSampler::SetHeightData(std::span<const float> grid, uint32_t width, uint32_t height, float scale) {
    m_grid = grid;
    m_width = width;
    m_height = height;
    m_scale = scale;
    m_invScale = (scale > 0.0f) ? (1.0f / scale) : 1.0f;
}

float TerrainHeightSampler::GetGridHeight(uint32_t x, uint32_t y) const noexcept {
    if (m_grid.empty()) {
        return 0.0f;
    }
    
    // Clamp to valid range to prevent out-of-bounds access
    x = std::clamp(x, 0u, m_width > 0 ? m_width - 1 : 0u);
    y = std::clamp(y, 0u, m_height > 0 ? m_height - 1 : 0u);
    
    // 2D grid index (C++23 style or standard multiplication)
    size_t index = static_cast<size_t>(y) * m_width + x;
    if (index < m_grid.size()) {
        return m_grid[index];
    }
    return 0.0f;
}

float TerrainHeightSampler::GetHeight(float worldX, float worldY) const noexcept {
    if (m_grid.empty() || m_width == 0 || m_height == 0) {
        return 0.0f;
    }

    // Convert world coordinates to grid space
    float gridX = worldX * m_invScale;
    float gridY = worldY * m_invScale;

    // Handle negative coordinates
    if (gridX < 0.0f) gridX = 0.0f;
    if (gridY < 0.0f) gridY = 0.0f;

    uint32_t x0 = static_cast<uint32_t>(gridX);
    uint32_t y0 = static_cast<uint32_t>(gridY);

    float fx = gridX - static_cast<float>(x0);
    float fy = gridY - static_cast<float>(y0);

    // Get heights at four corners
    float h00 = GetGridHeight(x0, y0);
    float h10 = GetGridHeight(x0 + 1, y0);
    float h01 = GetGridHeight(x0, y0 + 1);
    float h11 = GetGridHeight(x0 + 1, y0 + 1);

    // Bilinear interpolation
    float h0 = std::lerp(h00, h10, fx);
    float h1 = std::lerp(h01, h11, fx);

    return std::lerp(h0, h1, fy);
}

D3DVECTOR TerrainHeightSampler::GetNormal(float worldX, float worldY) const noexcept {
    if (m_grid.empty() || m_width == 0 || m_height == 0) {
        return {0.0f, 0.0f, 1.0f};
    }

    // Central difference method for normal calculation
    float hL = GetHeight(worldX - m_scale, worldY);
    float hR = GetHeight(worldX + m_scale, worldY);
    float hD = GetHeight(worldX, worldY - m_scale);
    float hU = GetHeight(worldX, worldY + m_scale);

    D3DVECTOR normal;
    normal.x = hL - hR;
    normal.y = hD - hU;
    normal.z = 2.0f * m_scale;

    // Normalize
    float lengthSq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
    if (lengthSq > 0.0f) {
        float invLength = 1.0f / std::sqrt(lengthSq);
        normal.x *= invLength;
        normal.y *= invLength;
        normal.z *= invLength;
    } else {
        normal = {0.0f, 0.0f, 1.0f};
    }

    return normal;
}

} // namespace EterLib::Render

