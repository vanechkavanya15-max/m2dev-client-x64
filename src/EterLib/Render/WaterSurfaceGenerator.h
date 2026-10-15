#pragma once

#include <vector>
#include <cstdint>
#include <d3d9.h>
#include <d3dx9.h>

namespace EterLib::Render
{
    struct D3DVERTEX
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DXVECTOR2 texcoord;
    };

    class WaterSurfaceGenerator
    {
    public:
        void BuildWaterPatch(float worldX, float worldY, float size, float height, std::vector<D3DVERTEX>& outVertices, std::vector<uint16_t>& outIndices);
        void UpdateWaveOffset(float timeSeconds) noexcept;

    private:
        float m_waveOffset = 0.0f;
    };
}

