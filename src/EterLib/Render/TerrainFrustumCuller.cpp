#include "TerrainFrustumCuller.h"

namespace EterLib::Render {

void TerrainFrustumCuller::SetFrustumPlanes(std::span<const D3DPLANE, 6> planes) {
    for (size_t i = 0; i < 6; ++i) {
        m_planes[i] = planes[i];
    }
}

bool TerrainFrustumCuller::IsBoxVisible(const TerrainAABB& box) const noexcept {
    // This logic exactly mirrors the optimized check in TerrainQuad_Traverse.cpp
    for (int i = 0; i < 6; ++i) {
        float px = (m_planes[i].a > 0.0f) ? box.maxX : box.minX;
        float py = (m_planes[i].b > 0.0f) ? box.maxY : box.minY;
        float pz = (m_planes[i].c > 0.0f) ? box.maxZ : box.minZ;
        
        float dotProduct = (m_planes[i].a * px) + (m_planes[i].b * py) + (m_planes[i].c * pz);
        
        if (dotProduct < -m_planes[i].d) {
            return false;
        }
    }
    return true;
}

void TerrainFrustumCuller::CullQuadTree(const TerrainAABB& root, std::vector<uint32_t>& visibleNodeIds) const {
    struct StackNode {
        TerrainAABB bounds;
        int depth;
        uint32_t patchIdStart;
    };
    
    // Max depth is 4 in TerrainQuadtreeCuller. A stack size of 32 is more than enough for depth 4 (max stack depth = 4 * 3 = 12).
    // Using 64 for extra safety.
    std::array<StackNode, 64> stack;
    int top = 0;
    
    // The root depth is 4 and starts with patchId 0 based on TerrainQuadtreeCuller::BuildQuadtree
    stack[top++] = {root, 4, 0};
    
    while (top > 0) {
        StackNode node = stack[--top];
        
        if (!IsBoxVisible(node.bounds)) {
            continue;
        }
        
        if (node.depth == 0) {
            visibleNodeIds.push_back(node.patchIdStart);
            continue;
        }
        
        float midX = (node.bounds.minX + node.bounds.maxX) * 0.5f;
        float midY = (node.bounds.minY + node.bounds.maxY) * 0.5f;
        
        // Depth-first post-order generation means a node at depth D generates 4^(D-1) leaves per child.
        // 1 << ((node.depth - 1) * 2) accurately computes 4^(D-1).
        uint32_t patchesPerChild = 1 << ((node.depth - 1) * 2);
        
        // In TerrainQuad_Traverse.cpp, BuildNodeRecursive creates children in the order:
        // 0: q1 (minX, minY to midX, midY)
        // 1: q2 (midX, minY to maxX, midY)
        // 2: q3 (minX, midY to midX, maxY)
        // 3: q4 (midX, midY to maxX, maxY)
        // Since we are using a stack (LIFO), we push them in reverse order (q4, q3, q2, q1)
        // so that q1 is popped and processed first, matching the recursive ID generation perfectly.
        
        // Push Child 3 (q4)
        stack[top++] = {
            {midX, midY, node.bounds.minZ, node.bounds.maxX, node.bounds.maxY, node.bounds.maxZ}, 
            node.depth - 1, 
            node.patchIdStart + patchesPerChild * 3
        };
                        
        // Push Child 2 (q3)
        stack[top++] = {
            {node.bounds.minX, midY, node.bounds.minZ, midX, node.bounds.maxY, node.bounds.maxZ}, 
            node.depth - 1, 
            node.patchIdStart + patchesPerChild * 2
        };
                        
        // Push Child 1 (q2)
        stack[top++] = {
            {midX, node.bounds.minY, node.bounds.minZ, node.bounds.maxX, midY, node.bounds.maxZ}, 
            node.depth - 1, 
            node.patchIdStart + patchesPerChild * 1
        };
                        
        // Push Child 0 (q1)
        stack[top++] = {
            {node.bounds.minX, node.bounds.minY, node.bounds.minZ, midX, midY, node.bounds.maxZ}, 
            node.depth - 1, 
            node.patchIdStart
        };
    }
}

} // namespace EterLib::Render

