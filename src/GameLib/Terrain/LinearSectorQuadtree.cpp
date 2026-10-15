#include "../StdAfx.h"
#include "LinearSectorQuadtree.h"
#include "TerrainEvents.h"
#include "../../EterBase/LogModern.h"
#include <algorithm>
#include <cmath>

namespace GameLib::Terrain {

    std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadtreeCuller() {
        return std::make_unique<LinearSectorQuadtree>();
    }

    void LinearSectorQuadtree::Clear() {
        m_nodes.fill(LinearQuadNode{});
        m_isBuilt = false;
        m_sectorCoord = SectorCoord{};
    }

    void LinearSectorQuadtree::BuildQuadtree(int32_t sectorX, int32_t sectorY) {
        BuildWithHeights(SectorCoord{sectorX, sectorY}, {}, 1.0f, 0.0f);
    }

    void LinearSectorQuadtree::BuildWithHeights(
        SectorCoord coord, 
        std::span<const uint16_t> heights, 
        float heightScale, 
        float baseHeight) noexcept 
    {
        Clear();
        m_sectorCoord = coord;
        WorldPosition basePos = TerrainMetrics::SectorToWorld(coord);

        uint32_t nextNodeIndex = 1;
        uint32_t leafPatchCounter = 0;

        BuildNodeRecursive(0, basePos.x, basePos.y, TerrainMetrics::SectorSize, 0,
                           -2000.0f, 5000.0f, nextNodeIndex, leafPatchCounter);

        // Jesli przekazano bufor wysokosci (np. 131x131 w formacie Metin2), oblicz dokladne AABB Z dla kazdego liscia
        if (!heights.empty() && heights.size() >= (TerrainMetrics::HeightmapSize * TerrainMetrics::HeightmapSize)) {
            // Liscie zaczynaja sie od indeksu: 1 (L0) + 4 (L1) + 16 (L2) = 21
            constexpr uint32_t leafStartIndex = 21;
            for (uint32_t i = 0; i < TotalLeaves; ++i) {
                uint32_t patchX = i % TerrainMetrics::PatchesPerSide;
                uint32_t patchY = i / TerrainMetrics::PatchesPerSide;
                CalculateLeafHeights(m_nodes[leafStartIndex + i], patchX, patchY, heights, heightScale, baseHeight);
            }

            // Propagacja minZ / maxZ w gore drzewa (od poziomu 2 do korzenia)
            // Poziom 2: wezly 5..20 (16 wezlow)
            for (uint32_t i = 5; i < 21; ++i) {
                uint16_t childBase = m_nodes[i].childBaseIndex;
                float minZ = m_nodes[childBase].minZ;
                float maxZ = m_nodes[childBase].maxZ;
                for (int c = 1; c < 4; ++c) {
                    minZ = std::min(minZ, m_nodes[childBase + c].minZ);
                    maxZ = std::max(maxZ, m_nodes[childBase + c].maxZ);
                }
                m_nodes[i].minZ = minZ;
                m_nodes[i].maxZ = maxZ;
            }

            // Poziom 1: wezly 1..4 (4 wezly)
            for (uint32_t i = 1; i < 5; ++i) {
                uint16_t childBase = m_nodes[i].childBaseIndex;
                float minZ = m_nodes[childBase].minZ;
                float maxZ = m_nodes[childBase].maxZ;
                for (int c = 1; c < 4; ++c) {
                    minZ = std::min(minZ, m_nodes[childBase + c].minZ);
                    maxZ = std::max(maxZ, m_nodes[childBase + c].maxZ);
                }
                m_nodes[i].minZ = minZ;
                m_nodes[i].maxZ = maxZ;
            }

            // Poziom 0: korzen (wezel 0)
            {
                uint16_t childBase = m_nodes[0].childBaseIndex;
                float minZ = m_nodes[childBase].minZ;
                float maxZ = m_nodes[childBase].maxZ;
                for (int c = 1; c < 4; ++c) {
                    minZ = std::min(minZ, m_nodes[childBase + c].minZ);
                    maxZ = std::max(maxZ, m_nodes[childBase + c].maxZ);
                }
                m_nodes[0].minZ = minZ;
                m_nodes[0].maxZ = maxZ;
            }
        }

        m_isBuilt = true;
        EterBase::ModernLogger::Info("LinearSectorQuadtree built for sector [{}, {}] ({} nodes, {} leaves).", 
            coord.x, coord.y, TotalNodes, TotalLeaves);

        QuadtreeBuiltEvent event{coord.x, coord.y, TotalNodes};
        UserInterface::Core::EventBus::GetInstance().Publish(event);
    }

    void LinearSectorQuadtree::BuildNodeRecursive(
        uint32_t nodeIndex, float x, float y, float size, size_t currentDepth,
        float minZ, float maxZ, uint32_t& nextFreeIndex, uint32_t& leafPatchCounter) noexcept 
    {
        auto& node = m_nodes[nodeIndex];
        node.minX = x;
        node.minY = y;
        node.minZ = minZ;
        node.maxX = x + size;
        node.maxY = y + size;
        node.maxZ = maxZ;
        node.lodLevel = static_cast<uint8_t>(currentDepth);

        if (currentDepth == Depth) {
            node.isLeaf = 1;
            node.childBaseIndex = 0;
            node.patchId = leafPatchCounter++;
            return;
        }

        node.isLeaf = 0;
        node.patchId = 0;
        node.childBaseIndex = static_cast<uint16_t>(nextFreeIndex);
        nextFreeIndex += 4;

        float halfSize = size * 0.5f;
        uint16_t base = node.childBaseIndex;

        BuildNodeRecursive(base + 0, x,            y,            halfSize, currentDepth + 1, minZ, maxZ, nextFreeIndex, leafPatchCounter);
        BuildNodeRecursive(base + 1, x + halfSize, y,            halfSize, currentDepth + 1, minZ, maxZ, nextFreeIndex, leafPatchCounter);
        BuildNodeRecursive(base + 2, x,            y + halfSize, halfSize, currentDepth + 1, minZ, maxZ, nextFreeIndex, leafPatchCounter);
        BuildNodeRecursive(base + 3, x + halfSize, y + halfSize, halfSize, currentDepth + 1, minZ, maxZ, nextFreeIndex, leafPatchCounter);
    }

    void LinearSectorQuadtree::CalculateLeafHeights(
        LinearQuadNode& node, 
        uint32_t patchX, 
        uint32_t patchY, 
        std::span<const uint16_t> heights, 
        float heightScale, 
        float baseHeight) noexcept 
    {
        // Kazdy patch w Metin2 ma 16x16 komorek, czyli (16+1)x(16+1) wierzcholkow
        uint32_t startCellX = patchX * TerrainMetrics::CellsPerPatch;
        uint32_t startCellY = patchY * TerrainMetrics::CellsPerPatch;
        uint32_t endCellX = startCellX + TerrainMetrics::CellsPerPatch;
        uint32_t endCellY = startCellY + TerrainMetrics::CellsPerPatch;

        float minZ = 1e9f;
        float maxZ = -1e9f;

        for (uint32_t cy = startCellY; cy <= endCellY; ++cy) {
            for (uint32_t cx = startCellX; cx <= endCellX; ++cx) {
                // Uwzglednienie marginesu +1 w formacie Metin2
                uint32_t rawX = cx + TerrainMetrics::RawBorderOffset;
                uint32_t rawY = cy + TerrainMetrics::RawBorderOffset;
                if (rawX < TerrainMetrics::HeightmapSize && rawY < TerrainMetrics::HeightmapSize) {
                    uint32_t idx = rawY * TerrainMetrics::HeightmapSize + rawX;
                    float h = baseHeight + (static_cast<float>(heights[idx]) * heightScale);
                    minZ = std::min(minZ, h);
                    maxZ = std::max(maxZ, h);
                }
            }
        }

        if (minZ <= maxZ) {
            node.minZ = minZ - 100.0f; // Margines bezpieczenstwa dla obiektow i trawy
            node.maxZ = maxZ + 200.0f;
        }
    }

    size_t LinearSectorQuadtree::CullTerrainPatches(
        const float viewFrustum[6][4], 
        uint32_t* outVisiblePatchIds) 
    {
        if (!m_isBuilt || !outVisiblePatchIds) return 0;

        // Plaski stos o stalym rozmiarze do eliminacji alokacji i rekurencji
        std::array<uint16_t, 64> nodeStack;
        int32_t stackTop = 0;
        nodeStack[0] = 0; // Rozpoczecie od korzenia

        size_t count = 0;

        while (stackTop >= 0) {
            uint16_t currentIdx = nodeStack[stackTop--];
            const auto& node = m_nodes[currentIdx];

            // Test AABB wzgledem 6 plaszczyzn Frustum (P-vertex test)
            bool outside = false;
            for (int i = 0; i < 6; ++i) {
                float px = (viewFrustum[i][0] > 0.0f) ? node.maxX : node.minX;
                float py = (viewFrustum[i][1] > 0.0f) ? node.maxY : node.minY;
                float pz = (viewFrustum[i][2] > 0.0f) ? node.maxZ : node.minZ;

                if ((viewFrustum[i][0] * px + viewFrustum[i][1] * py + viewFrustum[i][2] * pz + viewFrustum[i][3]) < 0.0f) {
                    outside = true;
                    break;
                }
            }

            if (outside) continue;

            if (node.isLeaf) {
                if (count < TotalLeaves) {
                    outVisiblePatchIds[count++] = node.patchId;
                }
            } else {
                // Dodanie 4 dzieci na stos
                for (int c = 0; c < 4; ++c) {
                    if (stackTop + 1 < static_cast<int32_t>(nodeStack.size())) {
                        nodeStack[++stackTop] = node.childBaseIndex + c;
                    }
                }
            }
        }

        TerrainCullCompletedEvent event{count};
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return count;
    }

    uint8_t LinearSectorQuadtree::SelectLOD(float distance) const {
        if (distance < 6400.0f) {
            return 0; // Najwyzszy detal
        }
        if (distance < 16000.0f) {
            return 1; // Sredni detal
        }
        return 2;     // Najnizszy detal
    }

} // namespace GameLib::Terrain
