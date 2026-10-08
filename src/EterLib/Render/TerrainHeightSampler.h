#pragma once

#include <span>
#include <cstdint>
#include <cmath>

#ifndef D3DVECTOR_DEFINED
#define D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
    float x;
    float y;
    float z;
} D3DVECTOR;
#endif

namespace EterLib::Render {

class TerrainHeightSampler {
public:
    TerrainHeightSampler() = default;
    ~TerrainHeightSampler() = default;

    void SetHeightData(std::span<const float> grid, uint32_t width, uint32_t height, float scale);

    [[nodiscard]] float GetHeight(float worldX, float worldY) const noexcept;
    [[nodiscard]] D3DVECTOR GetNormal(float worldX, float worldY) const noexcept;

private:
    std::span<const float> m_grid;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_scale = 1.0f;
    float m_invScale = 1.0f;

    [[nodiscard]] float GetGridHeight(uint32_t x, uint32_t y) const noexcept;
};

} // namespace EterLib::Render

