#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/Client/Mimic/SplineMotionInterpolator.h"
#include "../src/Client/Mimic/SplineMotionInterpolator.cpp" // Include cpp for standalone compilation in tests
#include <cmath>

using namespace Client::Mimic;

bool AreVectorsEqual(const Vector3& a, const Vector3& b, float epsilon = 0.001f) {
    return std::abs(a.x() - b.x()) < epsilon &&
           std::abs(a.y() - b.y()) < epsilon &&
           std::abs(a.z() - b.z()) < epsilon;
}

TEST_CASE("SplineMotionInterpolator Basic Operations") {
    SplineMotionInterpolator interpolator;
    
    CHECK(interpolator.GetPointCount() == 0);
    
    interpolator.AddPoint(Vector3(0.0f, 0.0f, 0.0f));
    CHECK(interpolator.GetPointCount() == 1);
    
    interpolator.Clear();
    CHECK(interpolator.GetPointCount() == 0);
    
    // Add points up to capacity
    for (size_t i = 0; i < SplineMotionInterpolator::kMaxPoints; ++i) {
        CHECK(interpolator.AddPoint(Vector3(static_cast<float>(i), 0.0f, 0.0f)));
    }
    
    CHECK(interpolator.GetPointCount() == SplineMotionInterpolator::kMaxPoints);
    
    // Should fail to add more
    CHECK_FALSE(interpolator.AddPoint(Vector3(100.0f, 100.0f, 100.0f)));
}

TEST_CASE("SplineMotionInterpolator Evaluation Matches Control Points") {
    SplineMotionInterpolator interpolator;
    interpolator.AddPoint(Vector3(0.0f, 0.0f, 0.0f));
    interpolator.AddPoint(Vector3(10.0f, 20.0f, 30.0f));
    interpolator.AddPoint(Vector3(40.0f, 50.0f, 60.0f));
    interpolator.AddPoint(Vector3(70.0f, 80.0f, 90.0f));

    // At t=0, 1, 2, 3, it should exactly match the control points
    Vector3 p0 = interpolator.Evaluate(0.0f);
    CHECK(AreVectorsEqual(p0, Vector3(0.0f, 0.0f, 0.0f)));

    Vector3 p1 = interpolator.Evaluate(1.0f);
    CHECK(AreVectorsEqual(p1, Vector3(10.0f, 20.0f, 30.0f)));

    Vector3 p2 = interpolator.Evaluate(2.0f);
    CHECK(AreVectorsEqual(p2, Vector3(40.0f, 50.0f, 60.0f)));

    Vector3 p3 = interpolator.Evaluate(3.0f);
    CHECK(AreVectorsEqual(p3, Vector3(70.0f, 80.0f, 90.0f)));
}

TEST_CASE("SplineMotionInterpolator C1 Derivative Continuity") {
    SplineMotionInterpolator interpolator;
    interpolator.AddPoint(Vector3(0.0f, 0.0f, 0.0f));
    interpolator.AddPoint(Vector3(10.0f, 5.0f, 0.0f));
    interpolator.AddPoint(Vector3(20.0f, -5.0f, 10.0f));
    interpolator.AddPoint(Vector3(30.0f, 15.0f, 20.0f));
    interpolator.AddPoint(Vector3(40.0f, 0.0f, 5.0f));

    // Check continuity of velocity at segment boundaries
    // The velocity as we approach a control point from the left should equal the velocity from the right
    
    auto check_continuity = [&](float t) {
        float epsilon = 0.0001f;
        Vector3 v_left = interpolator.EvaluateVelocity(t - epsilon);
        Vector3 v_right = interpolator.EvaluateVelocity(t + epsilon);
        Vector3 v_exact = interpolator.EvaluateVelocity(t);

        CHECK(AreVectorsEqual(v_left, v_exact, 0.1f));
        CHECK(AreVectorsEqual(v_right, v_exact, 0.1f));
    };

    // Check at control points index 1, 2, 3
    check_continuity(1.0f);
    check_continuity(2.0f);
    check_continuity(3.0f);
}

TEST_CASE("SplineMotionInterpolator Smoothness Anti-Cheat Validation") {
    // We want to ensure there are no sudden jumps in position (which anti-cheat detects)
    SplineMotionInterpolator interpolator;
    interpolator.AddPoint(Vector3(0.0f, 0.0f, 0.0f));
    interpolator.AddPoint(Vector3(100.0f, 0.0f, 0.0f));
    interpolator.AddPoint(Vector3(100.0f, 100.0f, 0.0f));
    interpolator.AddPoint(Vector3(0.0f, 100.0f, 0.0f));

    float max_dist_sq = 0.0f;
    Vector3 prev_pos = interpolator.Evaluate(0.0f);

    // Sample densely to find max distance between adjacent samples
    const int num_samples = 100;
    for (int i = 1; i <= num_samples; ++i) {
        float t = (3.0f * i) / num_samples; // 3 segments
        Vector3 curr_pos = interpolator.Evaluate(t);
        
        Vector3 diff = curr_pos - prev_pos;
        float dist_sq = diff.x() * diff.x() + diff.y() * diff.y() + diff.z() * diff.z();
        
        if (dist_sq > max_dist_sq) {
            max_dist_sq = dist_sq;
        }
        prev_pos = curr_pos;
    }

    // Since we sample 100 times over 3 segments (total distance roughly 300), 
    // step size is ~3. Distance squared should be roughly 9. We check it's bounded.
    // If there was an instant jump, max_dist_sq would be massive.
    CHECK(max_dist_sq < 25.0f); // 5^2, allowing for some curve overshoot
}
