#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <array>

#ifndef D3DPLANE_DEFINED
#define D3DPLANE_DEFINED
struct D3DPLANE { 
    float a, b, c, d; 
};
#endif

namespace EterLib::Render {

struct TerrainAABB {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
};

class TerrainFrustumCuller {
public:
    TerrainFrustumCuller() = default;
    ~TerrainFrustumCuller() = default;

    void SetFrustumPlanes(std::span<const D3DPLANE, 6> planes);
    [[nodiscard]] bool IsBoxVisible(const TerrainAABB& box) const noexcept;
    void CullQuadTree(const TerrainAABB& root, std::vector<uint32_t>& visibleNodeIds) const;

private:
    std::array<D3DPLANE, 6> m_planes{};
};

} // namespace EterLib::Render

