#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../UserInterface/Core/EventBus.h"
#include "TerrainEvents.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>
#include <memory>
#include <span>

namespace GameLib::Terrain
{
    /**
     * @brief Quadtree node structure for AABB culling.
     */
    struct QuadtreeNode
    {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
        bool isLeaf;
        uint32_t patchId;
        std::unique_ptr<QuadtreeNode> children[4];

        QuadtreeNode() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0), isLeaf(false), patchId(0) {}
    };

    /**
     * @brief Implementacja systemu wycinania (culling) dla elementow mapy oparta o QuadTree.
     * Zgodna z SRP, zapewnia integracje z EventBus i uzywa EterBase::Result.
     */
    class TerrainQuadtreeCuller final : public ITerrainQuadtreeCuller
    {
    public:
        TerrainQuadtreeCuller() = default;
        ~TerrainQuadtreeCuller() override { Clear(); }

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            auto result = ValidateSector(sectorX, sectorY);
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to build quadtree: {}", EterBase::ToString(result.error()));
                return;
            }

            EterBase::ModernLogger::Info("Building Quadtree for sector X: {}, Y: {}", sectorX, sectorY);
            
            m_sectorX = sectorX;
            m_sectorY = sectorY;
            m_isBuilt = true;

            // Zbudowanie prostego quadtree dla 8x8 = 64 patchow terenu w obrebie sektora.
            // Zalozenie: Sektor ma wymiary 12800 x 12800 jednostek, patch ma 1600 x 1600.
            float sMinX = static_cast<float>(sectorX * 12800);
            float sMinY = static_cast<float>(sectorY * 12800);
            m_root = BuildNode(sMinX, sMinY, 12800.0f, 0, 0, 8);

            EterBase::ModernLogger::Debug("Quadtree built successfully.");
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            if (!m_isBuilt || !m_root)
            {
                EterBase::ModernLogger::Warning("Quadtree not built, returning 0 visible patches.");
                return 0;
            }

            EterBase::ModernLogger::Trace("Culling patches against view frustum.");

            std::vector<uint32_t> visiblePatches;
            CullNode(m_root.get(), viewFrustum, visiblePatches);

            if (outVisiblePatchIds != nullptr)
            {
                for (size_t i = 0; i < visiblePatches.size(); ++i)
                {
                    outVisiblePatchIds[i] = visiblePatches[i];
                }
            }

            // Powiadomienie GUI / innych podsystemow o zakonczeniu cullingu
            TerrainCullCompletedEvent event(visiblePatches.size());
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return visiblePatches.size();
        }

        uint8_t SelectLOD(float distance) const override
        {
            if (distance < 3000.0f) return 0; // High detail
            if (distance < 8000.0f) return 1; // Medium detail
            return 2; // Low detail
        }

        void Clear() override
        {
            if (m_isBuilt)
            {
                EterBase::ModernLogger::Info("Clearing Terrain Quadtree Culler.");
                m_root.reset();
                m_isBuilt = false;
            }
        }

    private:
        std::unique_ptr<QuadtreeNode> BuildNode(float startX, float startY, float size, uint32_t patchX, uint32_t patchY, uint32_t patchCount)
        {
            auto node = std::make_unique<QuadtreeNode>();
            node->minX = startX;
            node->minY = startY;
            node->minZ = -2000.0f; // Domyslny Z dla przykladu
            node->maxX = startX + size;
            node->maxY = startY + size;
            node->maxZ = 2000.0f;

            if (patchCount <= 1)
            {
                node->isLeaf = true;
                node->patchId = patchY * 8 + patchX; // Mapowanie do ID patcha (0-63)
            }
            else
            {
                float halfSize = size * 0.5f;
                uint32_t halfCount = patchCount / 2;
                
                node->children[0] = BuildNode(startX,            startY,            halfSize, patchX,             patchY,             halfCount); // NW
                node->children[1] = BuildNode(startX + halfSize, startY,            halfSize, patchX + halfCount, patchY,             halfCount); // NE
                node->children[2] = BuildNode(startX,            startY + halfSize, halfSize, patchX,             patchY + halfCount, halfCount); // SW
                node->children[3] = BuildNode(startX + halfSize, startY + halfSize, halfSize, patchX + halfCount, patchY + halfCount, halfCount); // SE
            }

            return node;
        }

        void CullNode(const QuadtreeNode* node, const float frustum[6][4], std::vector<uint32_t>& outVisible) const
        {
            if (!node) return;

            if (!IsAABBVisible(node->minX, node->minY, node->minZ, node->maxX, node->maxY, node->maxZ, frustum))
            {
                return; // Odrzucone przez culling
            }

            if (node->isLeaf)
            {
                outVisible.push_back(node->patchId);
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                {
                    CullNode(node->children[i].get(), frustum, outVisible);
                }
            }
        }

        bool IsAABBVisible(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, const float frustum[6][4]) const
        {
            for (int i = 0; i < 6; ++i)
            {
                float px = (frustum[i][0] > 0.0f) ? maxX : minX;
                float py = (frustum[i][1] > 0.0f) ? maxY : minY;
                float pz = (frustum[i][2] > 0.0f) ? maxZ : minZ;

                float distance = frustum[i][0] * px + frustum[i][1] * py + frustum[i][2] * pz + frustum[i][3];
                if (distance < 0.0f)
                {
                    return false; // AABB znajduje sie poza jedna z plaszczyzn frustum
                }
            }
            return true;
        }

        EterBase::Result<void, EterBase::EntityError> ValidateSector(int32_t sectorX, int32_t sectorY) const
        {
            if (sectorX < 0 || sectorY < 0)
            {
                return EterBase::MakeError(EterBase::EntityError::OutOfRange);
            }
            return {};
        }

        int32_t m_sectorX = 0;
        int32_t m_sectorY = 0;
        bool m_isBuilt = false;
        std::unique_ptr<QuadtreeNode> m_root;
    };

    std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadtreeCuller()
    {
        return std::make_unique<TerrainQuadtreeCuller>();
    }
} // namespace GameLib::Terrain
