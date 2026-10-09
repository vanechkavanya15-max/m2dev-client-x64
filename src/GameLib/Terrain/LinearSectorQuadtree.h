#pragma once

#include "ITerrainQuadtreeCuller.h"
#include "TerrainCoordinates.h"
#include <array>
#include <span>
#include <cstdint>

namespace GameLib::Terrain {

    struct alignas(32) LinearQuadNode {
        float minX{0.0f}, minY{0.0f}, minZ{0.0f};
        float maxX{0.0f}, maxY{0.0f}, maxZ{0.0f};
        uint16_t childBaseIndex{0}; // 0 oznacza lisc
        uint8_t lodLevel{0};
        uint8_t isLeaf{1};
        uint32_t patchId{0};       // Indeks patcha w sektorze (0..63)
    };

    class LinearSectorQuadtree final : public ITerrainQuadtreeCuller {
    public:
        // Dla Metin2: Sektor 25600 ma 8x8 = 64 patche (PatchSize = 3200)
        // Poziom 0: 1 korzen (25600)
        // Poziom 1: 4 wezly (12800)
        // Poziom 2: 16 wezlow (6400)
        // Poziom 3: 64 liscie (3200) -> DOKLADNIE 64 patche sektora Metin2!
        static constexpr size_t Depth = 3;
        static constexpr size_t TotalNodes = 85; // 1 + 4 + 16 + 64 = 85
        static constexpr size_t TotalLeaves = 64;

        LinearSectorQuadtree() noexcept = default;
        ~LinearSectorQuadtree() override = default;

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override;

        void BuildWithHeights(
            SectorCoord coord, 
            std::span<const uint16_t> heights, 
            float heightScale = 1.0f, 
            float baseHeight = 0.0f) noexcept;

        size_t CullTerrainPatches(
            const float viewFrustum[6][4], 
            uint32_t* outVisiblePatchIds) override;

        uint8_t SelectLOD(float distance) const override;

        void Clear() override;

        [[nodiscard]] SectorCoord GetSectorCoord() const noexcept { return m_sectorCoord; }
        [[nodiscard]] bool IsBuilt() const noexcept { return m_isBuilt; }
        [[nodiscard]] const LinearQuadNode& GetNode(size_t index) const noexcept { return m_nodes[index]; }

    private:
        void BuildNodeRecursive(
            uint32_t nodeIndex, float x, float y, float size, size_t currentDepth,
            float minZ, float maxZ, uint32_t& nextFreeIndex, uint32_t& leafPatchCounter) noexcept;

        void CalculateLeafHeights(
            LinearQuadNode& node, 
            uint32_t patchX, 
            uint32_t patchY, 
            std::span<const uint16_t> heights, 
            float heightScale, 
            float baseHeight) noexcept;

        SectorCoord m_sectorCoord{};
        std::array<LinearQuadNode, TotalNodes> m_nodes{};
        bool m_isBuilt{false};
    };

} // namespace GameLib::Terrain
