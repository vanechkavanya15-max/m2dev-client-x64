#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"

#include <cmath>
#include <vector>
#include <array>
#include <span>
#include <memory>

#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

namespace {
    struct TerrainFogCullEvent : public UserInterface::Core::IEvent {
        uint32_t culledPatchCount;
        uint32_t visiblePatchCount;

        TerrainFogCullEvent(uint32_t culled, uint32_t visible)
            : culledPatchCount(culled), visiblePatchCount(visible) {}
    };

    constexpr float MAX_FOG_DISTANCE = 25000.0f; // 250 metrow w jednostkach gry
    constexpr float PATCH_SIZE = 3200.0f;        // Zwykle rozmiar patcha to 3200
    constexpr uint32_t PATCHES_PER_SIDE = 8;     // Typowo sektor ma 8x8 patch'y
    constexpr float SECTOR_SIZE = PATCH_SIZE * PATCHES_PER_SIDE; // 25600.0f

    struct BoundingBox {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
        
        bool IntersectsFrustum(const float frustum[6][4]) const {
            for (int i = 0; i < 6; ++i) {
                float px = (frustum[i][0] > 0.0f) ? maxX : minX;
                float py = (frustum[i][1] > 0.0f) ? maxY : minY;
                float pz = (frustum[i][2] > 0.0f) ? maxZ : minZ;
                
                float dot = (frustum[i][0] * px) + (frustum[i][1] * py) + (frustum[i][2] * pz) + frustum[i][3];
                if (dot < 0.0f) {
                    return false;
                }
            }
            return true;
        }

        bool IntersectsFog(float cameraX, float cameraY) const {
            float centerX = (minX + maxX) * 0.5f;
            float centerY = (minY + maxY) * 0.5f;
            float distanceSq = (centerX - cameraX) * (centerX - cameraX) + (centerY - cameraY) * (centerY - cameraY);
            return distanceSq <= (MAX_FOG_DISTANCE * MAX_FOG_DISTANCE);
        }
    };

    struct QuadNode {
        BoundingBox bounds;
        bool isLeaf;
        uint32_t patchId;
        std::unique_ptr<QuadNode> children[4];
    };
}

namespace GameLib::Terrain {

    class TerrainQuadFogCuller final : public ITerrainQuadtreeCuller {
    public:
        TerrainQuadFogCuller() {
            EterBase::ModernLogger::Info("TerrainQuadFogCuller: Inicjalizacja modulu (C++23)");
        }

        ~TerrainQuadFogCuller() override {
            Clear();
            EterBase::ModernLogger::Debug("TerrainQuadFogCuller: Zamkniecie modulu");
        }

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override {
            Clear();
            m_sectorX = sectorX;
            m_sectorY = sectorY;
            
            float basePathX = static_cast<float>(sectorX) * SECTOR_SIZE;
            float basePathY = static_cast<float>(sectorY) * SECTOR_SIZE;

            BoundingBox rootBounds = {
                basePathX, basePathY, -2000.0f,
                basePathX + SECTOR_SIZE, basePathY + SECTOR_SIZE, 2000.0f
            };

            m_root = BuildNode(rootBounds, 0, 0, PATCHES_PER_SIDE);
            m_isBuilt = true;

            EterBase::ModernLogger::Info("TerrainQuadFogCuller: Zbudowano quadtree dla sektora [{}, {}]", sectorX, sectorY);
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override {
            if (!m_isBuilt || !m_root) {
                EterBase::ModernLogger::Error("TerrainQuadFogCuller: Próba cullingu bez zbudowanego quadtree.");
                return 0;
            }
            
            if (!outVisiblePatchIds) {
                EterBase::ModernLogger::Error("TerrainQuadFogCuller: Bufor wynikowy nie zostal przekazany.");
                return 0;
            }

            uint32_t visibleCount = 0;
            uint32_t culledCount = 0;
            
            // Punkt odniesienia kamery wyliczany w uproszczeniu z centrum sektora (domyslnie by to szlo z kamery gracza)
            float cameraX = static_cast<float>(m_sectorX) * SECTOR_SIZE + (SECTOR_SIZE * 0.5f);
            float cameraY = static_cast<float>(m_sectorY) * SECTOR_SIZE + (SECTOR_SIZE * 0.5f);

            TraverseAndCull(m_root.get(), viewFrustum, cameraX, cameraY, outVisiblePatchIds, visibleCount, culledCount);
            
            TerrainFogCullEvent event(culledCount, visibleCount);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return static_cast<size_t>(visibleCount);
        }

        uint8_t SelectLOD(float distance) const override {
            if (distance < 5000.0f) return 0;
            if (distance < 12000.0f) return 1;
            if (distance < MAX_FOG_DISTANCE) return 2;
            return 3;
        }

        void Clear() override {
            m_root.reset();
            m_isBuilt = false;
        }

    private:
        std::unique_ptr<QuadNode> BuildNode(const BoundingBox& bounds, uint32_t offsetX, uint32_t offsetY, uint32_t size) {
            auto node = std::make_unique<QuadNode>();
            node->bounds = bounds;

            if (size == 1) {
                node->isLeaf = true;
                node->patchId = offsetY * PATCHES_PER_SIDE + offsetX;
                return node;
            }

            node->isLeaf = false;
            uint32_t halfSize = size / 2;
            float midX = (bounds.minX + bounds.maxX) * 0.5f;
            float midY = (bounds.minY + bounds.maxY) * 0.5f;

            BoundingBox b0 = { bounds.minX, bounds.minY, bounds.minZ, midX, midY, bounds.maxZ };
            BoundingBox b1 = { midX, bounds.minY, bounds.minZ, bounds.maxX, midY, bounds.maxZ };
            BoundingBox b2 = { bounds.minX, midY, bounds.minZ, midX, bounds.maxY, bounds.maxZ };
            BoundingBox b3 = { midX, midY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ };

            node->children[0] = BuildNode(b0, offsetX, offsetY, halfSize);
            node->children[1] = BuildNode(b1, offsetX + halfSize, offsetY, halfSize);
            node->children[2] = BuildNode(b2, offsetX, offsetY + halfSize, halfSize);
            node->children[3] = BuildNode(b3, offsetX + halfSize, offsetY + halfSize, halfSize);

            return node;
        }

        void TraverseAndCull(const QuadNode* node, const float viewFrustum[6][4], float cameraX, float cameraY, 
                             uint32_t* outVisiblePatchIds, uint32_t& visibleCount, uint32_t& culledCount) const {
            if (!node) return;

            if (!node->bounds.IntersectsFrustum(viewFrustum) || !node->bounds.IntersectsFog(cameraX, cameraY)) {
                // Jesli nie przecina frustum LUB jest calkowicie we mgle, wycinamy (cull)
                CountCulledLeaves(node, culledCount);
                return;
            }

            if (node->isLeaf) {
                outVisiblePatchIds[visibleCount++] = node->patchId;
            } else {
                for (int i = 0; i < 4; ++i) {
                    TraverseAndCull(node->children[i].get(), viewFrustum, cameraX, cameraY, outVisiblePatchIds, visibleCount, culledCount);
                }
            }
        }

        void CountCulledLeaves(const QuadNode* node, uint32_t& culledCount) const {
            if (!node) return;
            if (node->isLeaf) {
                culledCount++;
            } else {
                for (int i = 0; i < 4; ++i) {
                    CountCulledLeaves(node->children[i].get(), culledCount);
                }
            }
        }

        int32_t m_sectorX = 0;
        int32_t m_sectorY = 0;
        bool m_isBuilt = false;
        std::unique_ptr<QuadNode> m_root;
    };
    
    std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadFogCuller() {
        return std::make_unique<TerrainQuadFogCuller>();
    }

} // namespace GameLib::Terrain
