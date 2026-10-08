#define TEST_MODE_DISABLE_STDAFX 1
#include "doctest.h"
#include "../src/EterLib/Render/ActorLODSelector.h"

TEST_CASE("ActorLODSelector selects correct LOD based on distance squared")
{
    EterLib::Render::ActorLODSelector selector;
    selector.SetLODThresholds(10.0f, 20.0f); // lod1Dist = 10, lod2Dist = 20
    // lod1DistSq = 100
    // lod2DistSq = 400

    SUBCASE("Distance less than lod1 returns LOD 0")
    {
        CHECK(selector.SelectLOD(0.0f) == 0);
        CHECK(selector.SelectLOD(50.0f) == 0);
        CHECK(selector.SelectLOD(99.9f) == 0);
    }

    SUBCASE("Distance exactly at lod1 returns LOD 1")
    {
        CHECK(selector.SelectLOD(100.0f) == 1);
    }

    SUBCASE("Distance between lod1 and lod2 returns LOD 1")
    {
        CHECK(selector.SelectLOD(200.0f) == 1);
        CHECK(selector.SelectLOD(399.9f) == 1);
    }

    SUBCASE("Distance exactly at lod2 returns LOD 2")
    {
        CHECK(selector.SelectLOD(400.0f) == 2);
    }

    SUBCASE("Distance greater than lod2 returns LOD 2")
    {
        CHECK(selector.SelectLOD(500.0f) == 2);
        CHECK(selector.SelectLOD(10000.0f) == 2);
    }
}

