#include <iostream>
#include <cmath>
#include <cassert>
#include "Client/Gameplay/MovementInterpolationEngine.h"

// Simple mock of expected test conditions
void test_dead_reckoning() {
    Client::Gameplay::MovementInterpolationEngine engine;
    
    // Snapshot at t=1000: pos=(0,0,0), vel=(10,0,0) (10 units per second)
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, 0.0f, 1000);
    
    // Extrapolate to t=1500 (0.5 seconds later) -> pos=(5,0,0)
    auto pos = engine.GetInterpolatedPosition(1500);
    
    assert(std::abs(pos.x - 5.0f) < 0.001f);
    assert(std::abs(pos.y - 0.0f) < 0.001f);
    assert(std::abs(pos.z - 0.0f) < 0.001f);
    
    std::cout << "test_dead_reckoning passed." << std::endl;
}

void test_lerp_between_snapshots() {
    Client::Gameplay::MovementInterpolationEngine engine;
    
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1000);
    engine.AddSnapshot({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, 2000);
    
    // Interpolate at t=1500 (midway)
    auto pos = engine.GetInterpolatedPosition(1500);
    
    assert(std::abs(pos.x - 5.0f) < 0.001f);
    assert(std::abs(pos.y - 0.0f) < 0.001f);
    assert(std::abs(pos.z - 0.0f) < 0.001f);
    
    std::cout << "test_lerp_between_snapshots passed." << std::endl;
}

void test_slerp_rotation() {
    Client::Gameplay::MovementInterpolationEngine engine;
    
    // Test normal interpolation
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 10.0f, 1000);
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 30.0f, 2000);
    
    float rot1 = engine.GetInterpolatedRotation(1500);
    assert(std::abs(rot1 - 20.0f) < 0.001f);
    
    engine.Clear();
    
    // Test shortest path across 0 degree boundary
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 350.0f, 1000);
    engine.AddSnapshot({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 10.0f, 2000);
    
    float rot2 = engine.GetInterpolatedRotation(1500);
    assert(std::abs(rot2 - 0.0f) < 0.001f || std::abs(rot2 - 360.0f) < 0.001f);
    
    std::cout << "test_slerp_rotation passed." << std::endl;
}

int main() {
    std::cout << "Running MovementInterpolationEngine tests..." << std::endl;
    
    test_dead_reckoning();
    test_lerp_between_snapshots();
    test_slerp_rotation();
    
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
