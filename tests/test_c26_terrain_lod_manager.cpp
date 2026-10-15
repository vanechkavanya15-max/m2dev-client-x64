#include <doctest.h>
#include "../src/EterLib/Render/TerrainLODManager.h"
#include <array>

TEST_SUITE("TerrainLODManager")
{
    TEST_CASE("SetLODDistances and CalculateLOD")
    {
        EterLib::Render::TerrainLODManager lodManager;
        std::array<float, 4> distances = { 1000.0f, 2000.0f, 3000.0f, 4000.0f };
        lodManager.SetLODDistances(distances);

        CHECK(lodManager.CalculateLOD(500.0f) == 0);
        CHECK(lodManager.CalculateLOD(999.9f) == 0);
        
        CHECK(lodManager.CalculateLOD(1000.0f) == 1);
        CHECK(lodManager.CalculateLOD(1500.0f) == 1);
        CHECK(lodManager.CalculateLOD(1999.9f) == 1);
        
        CHECK(lodManager.CalculateLOD(2000.0f) == 2);
        CHECK(lodManager.CalculateLOD(2500.0f) == 2);
        CHECK(lodManager.CalculateLOD(2999.9f) == 2);
        
        CHECK(lodManager.CalculateLOD(3000.0f) == 3);
        CHECK(lodManager.CalculateLOD(4000.0f) == 3);
        CHECK(lodManager.CalculateLOD(5000.0f) == 3);
    }

    TEST_CASE("GetStitchMask")
    {
        EterLib::Render::TerrainLODManager lodManager;
        
        // No stitching needed
        CHECK(lodManager.GetStitchMask(0, 0, 0, 0, 0) == 0);
        CHECK(lodManager.GetStitchMask(1, 1, 1, 1, 1) == 0);
        CHECK(lodManager.GetStitchMask(2, 2, 2, 2, 2) == 0);
        CHECK(lodManager.GetStitchMask(3, 3, 3, 3, 3) == 0);

        // Lower LOD neighbors don't require stitching on our side
        CHECK(lodManager.GetStitchMask(1, 0, 0, 0, 0) == 0);
        CHECK(lodManager.GetStitchMask(2, 1, 0, 1, 0) == 0);
        
        // Higher LOD neighbors require stitching
        // North
        CHECK(lodManager.GetStitchMask(0, 1, 0, 0, 0) == 1);
        // South
        CHECK(lodManager.GetStitchMask(0, 0, 1, 0, 0) == 2);
        // East
        CHECK(lodManager.GetStitchMask(0, 0, 0, 1, 0) == 4);
        // West
        CHECK(lodManager.GetStitchMask(0, 0, 0, 0, 1) == 8);
        
        // Multiple higher LOD neighbors
        CHECK(lodManager.GetStitchMask(0, 1, 1, 0, 0) == 3); // North(1) | South(2)
        CHECK(lodManager.GetStitchMask(1, 2, 0, 2, 0) == 5); // North(1) | East(4)
        CHECK(lodManager.GetStitchMask(2, 3, 3, 3, 3) == 15); // All directions (1|2|4|8)
    }
}

