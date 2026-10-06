#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include <memory>
#include <vector>

namespace GameLib::Terrain
{
    namespace
    {
        struct AABB
        {
            float minX, minY, minZ;
            float maxX, maxY, maxZ;
        };

        struct QuadTreeNode
        {
            AABB bounds;
            int32_t patchId{-1};
            std::unique_ptr<QuadTreeNode> children[4];
            bool isLeaf{false};
        };

        bool IsAABBInFrustum(const AABB& aabb, const float frustum[6][4])
        {
            for (int i = 0; i < 6; ++i)
            {
                float px = (frustum[i][0] > 0.0f) ? aabb.maxX : aabb.minX;
                float py = (frustum[i][1] > 0.0f) ? aabb.maxY : aabb.minY;
                float pz = (frustum[i][2] > 0.0f) ? aabb.maxZ : aabb.minZ;

                float dotProduct = (frustum[i][0] * px) + (frustum[i][1] * py) + (frustum[i][2] * pz);
                if (dotProduct < -frustum[i][3])
                {
                    return false;
                }
            }
            return true;
        }

        void TraverseNode(const QuadTreeNode* node, const float frustum[6][4], uint32_t* outVisiblePatchIds, size_t& count)
        {
            if (!node)
            {
                return;
            }

            if (!IsAABBInFrustum(node->bounds, frustum))
            {
                return;
            }

            if (node->isLeaf)
            {
                if (node->patchId >= 0)
                {
                    outVisiblePatchIds[count++] = static_cast<uint32_t>(node->patchId);
                }
                return;
            }

            for (int i = 0; i < 4; ++i)
            {
                TraverseNode(node->children[i].get(), frustum, outVisiblePatchIds, count);
            }
        }
        
        std::unique_ptr<QuadTreeNode> BuildNodeRecursive(const AABB& bounds, int depth, int32_t& nextPatchId)
        {
            auto node = std::make_unique<QuadTreeNode>();
            node->bounds = bounds;
            
            if (depth == 0)
            {
                node->isLeaf = true;
                node->patchId = nextPatchId++;
                return node;
            }
            
            float midX = (bounds.minX + bounds.maxX) * 0.5f;
            float midY = (bounds.minY + bounds.maxY) * 0.5f;
            
            AABB q1 = {bounds.minX, bounds.minY, bounds.minZ, midX, midY, bounds.maxZ};
            AABB q2 = {midX, bounds.minY, bounds.minZ, bounds.maxX, midY, bounds.maxZ};
            AABB q3 = {bounds.minX, midY, bounds.minZ, midX, bounds.maxY, bounds.maxZ};
            AABB q4 = {midX, midY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ};
            
            node->children[0] = BuildNodeRecursive(q1, depth - 1, nextPatchId);
            node->children[1] = BuildNodeRecursive(q2, depth - 1, nextPatchId);
            node->children[2] = BuildNodeRecursive(q3, depth - 1, nextPatchId);
            node->children[3] = BuildNodeRecursive(q4, depth - 1, nextPatchId);
            
            return node;
        }
    }

    class TerrainQuadtreeCuller final : public ITerrainQuadtreeCuller
    {
    public:
        TerrainQuadtreeCuller()
        {
            EterBase::ModernLogger::Info("TerrainQuadtreeCuller created.");
        }

        ~TerrainQuadtreeCuller() override
        {
            Clear();
            EterBase::ModernLogger::Info("TerrainQuadtreeCuller destroyed.");
        }

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            EterBase::ModernLogger::Debug("Building Quadtree for sector [{}, {}]", sectorX, sectorY);
            
            // Generate a bounding box based on sector coordinates, using typical Metin2 map dimensions
            float minX = sectorX * 25600.0f;
            float minY = sectorY * 25600.0f;
            float maxX = minX + 25600.0f;
            float maxY = minY + 25600.0f;
            
            AABB sectorBounds = {minX, minY, -2000.0f, maxX, maxY, 2000.0f};
            
            int32_t nextPatchId = 0;
            // Build down to depth 4 (16x16 grid of patches = 256 patches)
            root_ = BuildNodeRecursive(sectorBounds, 4, nextPatchId);

            EterBase::ModernLogger::Debug("Quadtree build completed with {} patches.", nextPatchId);
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            if (!root_)
            {
                return 0;
            }

            size_t count = 0;
            TraverseNode(root_.get(), viewFrustum, outVisiblePatchIds, count);
            
            return count;
        }

        uint8_t SelectLOD(float distance) const override
        {
            if (distance < 1500.0f) return 0;
            if (distance < 3000.0f) return 1;
            return 2;
        }

        void Clear() override
        {
            if (root_)
            {
                EterBase::ModernLogger::Debug("Clearing Quadtree data.");
                root_.reset();
            }
        }

    private:
        std::unique_ptr<QuadTreeNode> root_;
    };
    
    // Factory function to allow instantiation of the culler
    std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadtreeCuller()
    {
        return std::make_unique<TerrainQuadtreeCuller>();
    }
}
