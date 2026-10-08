#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Client/World/InstancePhysicsEngine.h"
#include <cmath>

using namespace Client::World;

TEST_SUITE("InstancePhysicsEngine") {
    TEST_CASE("Interpolation accuracy at halfway points") {
        InstancePhysicsEngine engine;
        engine.PushSnapshot({1000, {0.0f, 0.0f, 0.0f}, 0.0f});
        engine.PushSnapshot({2000, {10.0f, 20.0f, 30.0f}, 100.0f});

        // Test halfway point
        auto pos = engine.InterpolatePosition(1500);
        CHECK(pos.x == doctest::Approx(5.0f));
        CHECK(pos.y == doctest::Approx(10.0f));
        CHECK(pos.z == doctest::Approx(15.0f));

        auto rot = engine.InterpolateRotation(1500);
        CHECK(rot == doctest::Approx(50.0f));
    }

    TEST_CASE("Interpolation exact bounds") {
        InstancePhysicsEngine engine;
        engine.PushSnapshot({1000, {0.0f, 0.0f, 0.0f}, 0.0f});
        engine.PushSnapshot({2000, {10.0f, 20.0f, 30.0f}, 100.0f});

        // Test lower bound
        auto pos1 = engine.InterpolatePosition(1000);
        CHECK(pos1.x == doctest::Approx(0.0f));
        
        // Test upper bound
        auto pos2 = engine.InterpolatePosition(2000);
        CHECK(pos2.x == doctest::Approx(10.0f));

        // Test out of bounds (should clamp to ends)
        auto pos3 = engine.InterpolatePosition(500);
        CHECK(pos3.x == doctest::Approx(0.0f));

        auto pos4 = engine.InterpolatePosition(2500);
        CHECK(pos4.x == doctest::Approx(10.0f));
    }

    TEST_CASE("Rotation shortest path (slerp)") {
        InstancePhysicsEngine engine;
        engine.PushSnapshot({1000, {0.0f, 0.0f, 0.0f}, 170.0f});
        engine.PushSnapshot({2000, {0.0f, 0.0f, 0.0f}, -170.0f}); // Shortest path is through 180/-180

        // Halfway point should be roughly 180 or -180 (equivalent)
        auto rot = engine.InterpolateRotation(1500);
        // It travels 170 -> 180 (-180) -> -170, a total distance of 20 degrees. Halfway is 180.
        CHECK(std::abs(std::abs(rot) - 180.0f) < 0.1f);
    }

    TEST_CASE("2D and 3D Euclidean distances") {
        Vector3f a{0.0f, 0.0f, 0.0f};
        Vector3f b{3.0f, 4.0f, 0.0f}; // 2D dist = 5
        Vector3f c{0.0f, 0.0f, 5.0f}; // 3D dist from a = 5

        CHECK(InstancePhysicsEngine::CalculateDistance2d(a, b) == doctest::Approx(5.0f));
        CHECK(InstancePhysicsEngine::CalculateDistanceSq3d(a, b) == doctest::Approx(25.0f));
        CHECK(InstancePhysicsEngine::CalculateDistanceSq3d(a, c) == doctest::Approx(25.0f));
    }

    TEST_CASE("IsInRange for within and outside max distance limits") {
        Vector3f a{0.0f, 0.0f, 0.0f};
        Vector3f b{10.0f, 0.0f, 0.0f};

        CHECK(InstancePhysicsEngine::IsInRange(a, b, 10.0f) == true);
        CHECK(InstancePhysicsEngine::IsInRange(a, b, 10.5f) == true);
        CHECK(InstancePhysicsEngine::IsInRange(a, b, 9.5f) == false);
    }

    TEST_CASE("Resetting the state") {
        InstancePhysicsEngine engine;
        engine.PushSnapshot({1000, {1.0f, 1.0f, 1.0f}, 0.0f});
        
        // After push, it should return position (1,1,1)
        auto pos1 = engine.InterpolatePosition(1000);
        CHECK(pos1.x == doctest::Approx(1.0f));

        engine.Reset();
        
        // After reset, no snapshots, should return (0,0,0)
        auto pos2 = engine.InterpolatePosition(1000);
        CHECK(pos2.x == doctest::Approx(0.0f));
    }

    TEST_CASE("Snapshot ring buffer limit (32 samples)") {
        InstancePhysicsEngine engine;
        for (int i = 0; i < 40; ++i) {
            engine.PushSnapshot({static_cast<uint64_t>(1000 + i * 100), {static_cast<float>(i), 0.0f, 0.0f}, 0.0f});
        }

        // Only the last 32 should remain.
        // Oldest should be index 8 (timestamp 1800)
        // If we query at timestamp 1700, we'll get the oldest available (1800 clamped)
        auto pos = engine.InterpolatePosition(1700);
        CHECK(pos.x == doctest::Approx(8.0f));
    }
}
