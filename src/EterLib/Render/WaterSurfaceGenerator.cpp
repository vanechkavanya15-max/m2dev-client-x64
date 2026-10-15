#include "WaterSurfaceGenerator.h"
#include <cmath>

namespace EterLib::Render
{
    void WaterSurfaceGenerator::BuildWaterPatch(float worldX, float worldY, float size, float height, std::vector<D3DVERTEX>& outVertices, std::vector<uint16_t>& outIndices)
    {
        outVertices.clear();
        outIndices.clear();

        const int numGridCells = 16;
        const float cellSize = size / numGridCells;
        
        for (int y = 0; y <= numGridCells; ++y)
        {
            for (int x = 0; x <= numGridCells; ++x)
            {
                float px = worldX + x * cellSize;
                float py = worldY + y * cellSize;
                
                // Simple sine wave displacement based on the global offset
                float displacement = std::sin(px * 0.1f + m_waveOffset) * std::cos(py * 0.1f + m_waveOffset) * 5.0f;
                
                D3DVERTEX v;
                v.position = D3DXVECTOR3(px, py, height + displacement);
                v.normal = D3DXVECTOR3(0.0f, 0.0f, 1.0f); // Simplification, would need actual normal calculation
                v.texcoord = D3DXVECTOR2(x / (float)numGridCells, y / (float)numGridCells);
                
                outVertices.push_back(v);
            }
        }
        
        for (int y = 0; y < numGridCells; ++y)
        {
            for (int x = 0; x < numGridCells; ++x)
            {
                uint16_t i0 = y * (numGridCells + 1) + x;
                uint16_t i1 = i0 + 1;
                uint16_t i2 = (y + 1) * (numGridCells + 1) + x;
                uint16_t i3 = i2 + 1;
                
                // Triangle 1
                outIndices.push_back(i0);
                outIndices.push_back(i1);
                outIndices.push_back(i2);
                
                // Triangle 2
                outIndices.push_back(i1);
                outIndices.push_back(i3);
                outIndices.push_back(i2);
            }
        }
    }

    void WaterSurfaceGenerator::UpdateWaveOffset(float timeSeconds) noexcept
    {
        m_waveOffset = timeSeconds;
    }
}

