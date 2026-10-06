#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <string_view>
#include <expected>

namespace GameLib::Terrain {

// Zgodnie z zero-conflict zasada, definiujemy event powiazany z budowa drzewa
// w pliku implementacyjnym, poniewaz nie mamy dedykowanego naglowka.
struct QuadtreeBuiltEvent : public UserInterface::Core::IEvent {
    int32_t sectorX;
    int32_t sectorY;
    size_t totalNodes;

    QuadtreeBuiltEvent(int32_t x, int32_t y, size_t count)
        : sectorX(x), sectorY(y), totalNodes(count) {}
};

struct QuadtreeNode {
    float minX, minY, minZ, maxX, maxY, maxZ;
    uint32_t patchId;
    bool isLeaf;
    std::unique_ptr<QuadtreeNode> children[4];

    QuadtreeNode() 
        : minX(0), minY(0), minZ(-10000.0f), maxX(0), maxY(0), maxZ(10000.0f)
        , patchId(0), isLeaf(true) {}
};

class TerrainQuadtreeCuller final : public ITerrainQuadtreeCuller {
public:
    TerrainQuadtreeCuller() = default;
    ~TerrainQuadtreeCuller() override = default;

    void BuildQuadtree(int32_t sectorX, int32_t sectorY) override;
    size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override;
    uint8_t SelectLOD(float distance) const override;
    void Clear() override;

private:
    std::expected<void, EterBase::NavigationError> BuildRecursive(std::unique_ptr<QuadtreeNode>& node, float x, float y, float size, int depth, int32_t sectorX, int32_t sectorY);
    void CullRecursive(const QuadtreeNode* node, const float frustum[6][4], uint32_t* outPatchIds, size_t& count);
    
    std::unique_ptr<QuadtreeNode> m_root;
    size_t m_nodeCount = 0;
};

constexpr int MAX_QUADTREE_DEPTH = 4;
constexpr float SECTOR_SIZE = 25600.0f; 

// Global factory function (standard C++ linkage to avoid C4190 with std::unique_ptr)
std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadtreeCuller() {
    return std::make_unique<TerrainQuadtreeCuller>();
}

void TerrainQuadtreeCuller::BuildQuadtree(int32_t sectorX, int32_t sectorY) {
    Clear();
    
    m_root = std::make_unique<QuadtreeNode>();
    
    float baseWorldX = static_cast<float>(sectorX) * SECTOR_SIZE;
    float baseWorldY = static_cast<float>(sectorY) * SECTOR_SIZE;
    
    auto result = BuildRecursive(m_root, baseWorldX, baseWorldY, SECTOR_SIZE, 0, sectorX, sectorY);
    if (!result) {
        EterBase::ModernLogger::Error("Failed to build quadtree: Navigation error occurred.");
        return;
    }

    EterBase::ModernLogger::Info("Quadtree built for sector [{}, {}] with {} nodes including static objects.", sectorX, sectorY, m_nodeCount);

    QuadtreeBuiltEvent event{sectorX, sectorY, m_nodeCount};
    UserInterface::Core::EventBus::GetInstance().Publish(event);
}

std::expected<void, EterBase::NavigationError> TerrainQuadtreeCuller::BuildRecursive(std::unique_ptr<QuadtreeNode>& node, float x, float y, float size, int depth, int32_t sectorX, int32_t sectorY) {
    if (!node) {
        return EterBase::MakeError(EterBase::NavigationError::BlockedTerrain);
    }

    m_nodeCount++;
    
    node->minX = x;
    node->minY = y;
    node->minZ = -10000.0f; 
    node->maxX = x + size;
    node->maxY = y + size;
    node->maxZ = 10000.0f;

    if (depth >= MAX_QUADTREE_DEPTH) {
        node->isLeaf = true;
        
        float patchLocalX = (x - (static_cast<float>(sectorX) * SECTOR_SIZE));
        float patchLocalY = (y - (static_cast<float>(sectorY) * SECTOR_SIZE));
        
        float leafSize = SECTOR_SIZE / static_cast<float>(1 << MAX_QUADTREE_DEPTH); 
        
        uint32_t xIndex = static_cast<uint32_t>(patchLocalX / leafSize);
        uint32_t yIndex = static_cast<uint32_t>(patchLocalY / leafSize);
        
        node->patchId = (yIndex << 16) | xIndex; 
        
        return {};
    }

    node->isLeaf = false;
    float halfSize = size * 0.5f;

    node->children[0] = std::make_unique<QuadtreeNode>(); 
    auto res1 = BuildRecursive(node->children[0], x, y, halfSize, depth + 1, sectorX, sectorY);
    if (!res1) return res1;

    node->children[1] = std::make_unique<QuadtreeNode>(); 
    auto res2 = BuildRecursive(node->children[1], x + halfSize, y, halfSize, depth + 1, sectorX, sectorY);
    if (!res2) return res2;

    node->children[2] = std::make_unique<QuadtreeNode>(); 
    auto res3 = BuildRecursive(node->children[2], x, y + halfSize, halfSize, depth + 1, sectorX, sectorY);
    if (!res3) return res3;

    node->children[3] = std::make_unique<QuadtreeNode>(); 
    auto res4 = BuildRecursive(node->children[3], x + halfSize, y + halfSize, halfSize, depth + 1, sectorX, sectorY);
    if (!res4) return res4;

    return {};
}

size_t TerrainQuadtreeCuller::CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) {
    if (!m_root || !outVisiblePatchIds) {
        return 0;
    }

    size_t count = 0;
    CullRecursive(m_root.get(), viewFrustum, outVisiblePatchIds, count);
    return count;
}

void TerrainQuadtreeCuller::CullRecursive(const QuadtreeNode* node, const float frustum[6][4], uint32_t* outPatchIds, size_t& count) {
    if (!node) {
        return;
    }

    bool outside = false;
    for (int p = 0; p < 6; ++p) {
        float vX = (frustum[p][0] > 0.0f) ? node->maxX : node->minX;
        float vY = (frustum[p][1] > 0.0f) ? node->maxY : node->minY;
        float vZ = (frustum[p][2] > 0.0f) ? node->maxZ : node->minZ;
        
        float dist = frustum[p][0] * vX + frustum[p][1] * vY + frustum[p][2] * vZ + frustum[p][3]; 
        
        if (dist < 0.0f) {
            outside = true;
            break;
        }
    }

    if (outside) {
        return;
    }

    if (node->isLeaf) {
        outPatchIds[count++] = node->patchId;
    } else {
        for (int i = 0; i < 4; ++i) {
            CullRecursive(node->children[i].get(), frustum, outPatchIds, count);
        }
    }
}

uint8_t TerrainQuadtreeCuller::SelectLOD(float distance) const {
    if (distance < 3000.0f) {
        return 0;
    } else if (distance < 8000.0f) {
        return 1;
    } else if (distance < 15000.0f) {
        return 2;
    }
    
    return 3; 
}

void TerrainQuadtreeCuller::Clear() {
    m_root.reset();
    m_nodeCount = 0;
}

} // namespace GameLib::Terrain
