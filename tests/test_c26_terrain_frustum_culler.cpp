#include "../src/EterLib/Render/TerrainFrustumCuller.h"
#include <iostream>
#include <cassert>

using namespace EterLib::Render;

int main() {
    std::cout << "[TEST] Starting TerrainFrustumCuller tests..." << std::endl;

    TerrainFrustumCuller culler;
    
    // Create a simple view frustum looking towards +Z
    // For simplicity, we just set up planes that form a box:
    // +X (Right), -X (Left), +Y (Top), -Y (Bottom), +Z (Far), -Z (Near)
    std::array<D3DPLANE, 6> planes = {{
        { -1.0f,  0.0f,  0.0f, 100.0f }, // Right plane (x < 100)
        {  1.0f,  0.0f,  0.0f, 100.0f }, // Left plane (x > -100)
        {  0.0f, -1.0f,  0.0f, 100.0f }, // Top plane (y < 100)
        {  0.0f,  1.0f,  0.0f, 100.0f }, // Bottom plane (y > -100)
        {  0.0f,  0.0f, -1.0f, 1000.0f}, // Far plane (z < 1000)
        {  0.0f,  0.0f,  1.0f, -10.0f }  // Near plane (z > 10)
    }};

    culler.SetFrustumPlanes(planes);

    // Test 1: AABB fully inside frustum
    {
        TerrainAABB insideBox = { -50.0f, -50.0f, 50.0f, 50.0f, 50.0f, 150.0f };
        bool visible = culler.IsBoxVisible(insideBox);
        if (!visible) {
            std::cerr << "[TEST FAILED] insideBox should be visible." << std::endl;
            return 1;
        }
        
        std::vector<uint32_t> visibleIds;
        culler.CullQuadTree(insideBox, visibleIds);
        
        // Since it's fully inside, all 4^4 = 256 patches should be visible
        if (visibleIds.size() != 256) {
            std::cerr << "[TEST FAILED] insideBox should return 256 visible patch IDs, got: " << visibleIds.size() << std::endl;
            return 1;
        }
        std::cout << "[TEST PASSED] insideBox generated " << visibleIds.size() << " visible nodes." << std::endl;
    }

    // Test 2: AABB completely outside frustum (behind camera)
    {
        TerrainAABB outsideBox = { -50.0f, -50.0f, -200.0f, 50.0f, 50.0f, -100.0f };
        bool visible = culler.IsBoxVisible(outsideBox);
        if (visible) {
            std::cerr << "[TEST FAILED] outsideBox should NOT be visible." << std::endl;
            return 1;
        }
        
        std::vector<uint32_t> visibleIds;
        culler.CullQuadTree(outsideBox, visibleIds);
        
        if (!visibleIds.empty()) {
            std::cerr << "[TEST FAILED] outsideBox should return 0 visible patch IDs, got: " << visibleIds.size() << std::endl;
            return 1;
        }
        std::cout << "[TEST PASSED] outsideBox generated 0 visible nodes." << std::endl;
    }
    
    // Test 3: Partial intersection (only root is partially visible, half space visible)
    {
        // Box spans from x=-200 to 0 (frustum starts at x=-100)
        TerrainAABB partialBox = { -200.0f, -50.0f, 50.0f, 0.0f, 50.0f, 150.0f };
        bool visible = culler.IsBoxVisible(partialBox);
        if (!visible) {
            std::cerr << "[TEST FAILED] partialBox should be visible." << std::endl;
            return 1;
        }
        
        std::vector<uint32_t> visibleIds;
        culler.CullQuadTree(partialBox, visibleIds);
        
        if (visibleIds.empty() || visibleIds.size() == 256) {
            std::cerr << "[TEST FAILED] partialBox should return some but not all visible patches, got: " << visibleIds.size() << std::endl;
            return 1;
        }
        std::cout << "[TEST PASSED] partialBox generated " << visibleIds.size() << " visible nodes." << std::endl;
    }

    std::cout << "[TEST] All tests passed successfully." << std::endl;
    return 0;
}

