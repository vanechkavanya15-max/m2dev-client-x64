#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include "TerrainSplatWeightMap.h"
#include <algorithm>

namespace EterLib::Render {

void TerrainSplatWeightMap::Initialize(uint32_t patchX, uint32_t patchY) {
    m_width = patchX;
    m_height = patchY;
    m_weights.assign(patchX * patchY, 0x00000000); // 0,0,0,0 initially
}

void TerrainSplatWeightMap::SetLayerWeight(uint32_t layerIdx, uint8_t weight) {
    if (layerIdx > 3) return;
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            SetLayerWeight(x, y, layerIdx, weight);
        }
    }
}

void TerrainSplatWeightMap::SetLayerWeight(uint32_t x, uint32_t y, uint32_t layerIdx, uint8_t weight) {
    if (layerIdx > 3 || x >= m_width || y >= m_height) return;

    uint32_t& packed = m_weights[y * m_width + x];
    
    uint8_t w[4];
    w[0] = static_cast<uint8_t>(packed & 0xFF);
    w[1] = static_cast<uint8_t>((packed >> 8) & 0xFF);
    w[2] = static_cast<uint8_t>((packed >> 16) & 0xFF);
    w[3] = static_cast<uint8_t>((packed >> 24) & 0xFF);

    uint32_t sumOther = 0;
    for (uint32_t i = 0; i < 4; ++i) {
        if (i != layerIdx) {
            sumOther += w[i];
        }
    }

    // Capture old weights for remainder distribution
    uint8_t oldW[4];
    for (int i = 0; i < 4; ++i) oldW[i] = w[i];

    w[layerIdx] = weight;
    int32_t remaining = 255 - weight;

    if (sumOther > 0) {
        uint32_t newSumOther = 0;
        for (uint32_t i = 0; i < 4; ++i) {
            if (i != layerIdx) {
                uint32_t scaled = (static_cast<uint32_t>(w[i]) * remaining) / sumOther;
                w[i] = static_cast<uint8_t>(scaled);
                newSumOther += w[i];
            }
        }
        
        // Distribute any remainder due to integer division based on original non-zero weights
        int32_t diff = remaining - newSumOther;
        for (uint32_t i = 0; i < 4 && diff > 0; ++i) {
            if (i != layerIdx && oldW[i] > 0) {
                w[i]++;
                diff--;
            }
        }
        // If there is still a difference, distribute it blindly (fallback)
        for (uint32_t i = 0; i < 4 && diff > 0; ++i) {
            if (i != layerIdx) {
                w[i]++;
                diff--;
            }
        }
    } else {
        // If sumOther is 0, we distribute the remaining into lowest index available.
        for (uint32_t i = 0; i < 4; ++i) {
            if (i != layerIdx) {
                w[i] = static_cast<uint8_t>(remaining);
                break;
            }
        }
    }

    packed = (static_cast<uint32_t>(w[0])) |
             (static_cast<uint32_t>(w[1]) << 8) |
             (static_cast<uint32_t>(w[2]) << 16) |
             (static_cast<uint32_t>(w[3]) << 24);
}

uint32_t TerrainSplatWeightMap::GetPackedWeight(uint32_t x, uint32_t y) const noexcept {
    if (x >= m_width || y >= m_height) return 0;
    return m_weights[y * m_width + x];
}

std::span<const uint32_t> TerrainSplatWeightMap::GetRawData() const noexcept {
    return {m_weights.data(), m_weights.size()};
}

} // namespace EterLib::Render

