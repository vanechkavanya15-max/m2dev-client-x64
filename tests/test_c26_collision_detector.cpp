#include <iostream>
#include <cassert>
#include <cmath>
#include "../src/Client/World/CollisionDetector.h"

using namespace Client::World;

void TestVector3() {
    Vector3 v1{1.0f, 2.0f, 3.0f};
    Vector3 v2{4.0f, 5.0f, 6.0f};

    Vector3 add = v1 + v2;
    assert(add.x == 5.0f && add.y == 7.0f && add.z == 9.0f);

    Vector3 sub = v2 - v1;
    assert(sub.x == 3.0f && sub.y == 3.0f && sub.z == 3.0f);

    Vector3 mul = v1 * 2.0f;
    assert(mul.x == 2.0f && mul.y == 4.0f && mul.z == 6.0f);

    float dot = v1.Dot(v2);
    assert(dot == 1.0f * 4.0f + 2.0f * 5.0f + 3.0f * 6.0f);

    assert(v1.LengthSq() == 1.0f + 4.0f + 9.0f);
    assert(std::abs(v1.Length() - std::sqrt(14.0f)) < 0.0001f);

    Vector3 norm = v1.Normalized();
    assert(std::abs(norm.Length() - 1.0f) < 0.0001f);

    std::cout << "TestVector3 passed!" << std::endl;
}

void TestCylinderCylinder() {
    Cylinder a{{0.0f, 0.0f, 0.0f}, 1.0f, 2.0f};
    Cylinder b{{1.5f, 0.0f, 0.0f}, 1.0f, 2.0f};

    // Intersection
    CollisionResult res1 = CollisionDetector::TestCylinderCylinder(a, b);
    assert(res1.hasCollision);
    assert(std::abs(res1.penetrationDepth - 0.5f) < 0.0001f);
    assert(std::abs(res1.contactNormal.x - 1.0f) < 0.0001f);

    // No intersection (distance > sum of radii)
    Cylinder c{{3.0f, 0.0f, 0.0f}, 1.0f, 2.0f};
    CollisionResult res2 = CollisionDetector::TestCylinderCylinder(a, c);
    assert(!res2.hasCollision);

    // No intersection (Z axis difference)
    Cylinder d{{0.0f, 0.0f, 3.0f}, 1.0f, 2.0f};
    CollisionResult res3 = CollisionDetector::TestCylinderCylinder(a, d);
    assert(!res3.hasCollision);

    std::cout << "TestCylinderCylinder passed!" << std::endl;
}

void TestCylinderAABB() {
    Cylinder cyl{{0.0f, 0.0f, 0.0f}, 1.0f, 2.0f};
    AABB aabb{{0.5f, -1.0f, 0.0f}, {2.0f, 1.0f, 2.0f}};

    // Intersection
    CollisionResult res1 = CollisionDetector::TestCylinderAABB(cyl, aabb);
    assert(res1.hasCollision);
    assert(std::abs(res1.contactNormal.x - 1.0f) < 0.0001f);
    assert(std::abs(res1.penetrationDepth - 0.5f) < 0.0001f);

    // No intersection
    AABB aabb2{{1.5f, -1.0f, 0.0f}, {3.0f, 1.0f, 2.0f}};
    CollisionResult res2 = CollisionDetector::TestCylinderAABB(cyl, aabb2);
    assert(!res2.hasCollision);

    // No intersection (Z axis)
    AABB aabb3{{0.5f, -1.0f, 3.0f}, {2.0f, 1.0f, 5.0f}};
    CollisionResult res3 = CollisionDetector::TestCylinderAABB(cyl, aabb3);
    assert(!res3.hasCollision);

    // Inside AABB
    Cylinder cyl2{{1.0f, 0.0f, 0.0f}, 0.1f, 2.0f};
    CollisionResult res4 = CollisionDetector::TestCylinderAABB(cyl2, aabb);
    assert(res4.hasCollision);

    std::cout << "TestCylinderAABB passed!" << std::endl;
}

void TestSlideVector() {
    Vector3 velocity{1.0f, -1.0f, 0.0f};
    Vector3 wallNormal{0.0f, 1.0f, 0.0f};

    Vector3 slide = CollisionDetector::CalculateSlideVector(velocity, wallNormal);
    assert(std::abs(slide.x - 1.0f) < 0.0001f);
    assert(std::abs(slide.y) < 0.0001f);
    assert(std::abs(slide.z) < 0.0001f);

    // Moving away from wall
    Vector3 velocity2{1.0f, 1.0f, 0.0f};
    Vector3 slide2 = CollisionDetector::CalculateSlideVector(velocity2, wallNormal);
    assert(std::abs(slide2.x - 1.0f) < 0.0001f);
    assert(std::abs(slide2.y - 1.0f) < 0.0001f);
    assert(std::abs(slide2.z) < 0.0001f);

    std::cout << "TestSlideVector passed!" << std::endl;
}

void TestResolveMovement() {
    Cylinder player{{0.0f, 0.0f, 0.0f}, 1.0f, 2.0f};
    Vector3 velocity{2.0f, 0.0f, 0.0f};

    std::vector<Cylinder> actors;
    std::vector<AABB> env = { {{2.5f, -1.0f, 0.0f}, {3.5f, 1.0f, 2.0f}} };

    Vector3 resolved = CollisionDetector::ResolveMovement(player, velocity, actors, env);
    
    // Player should slide to stop or have reduced velocity in x
    // Original pos: 0. Vel: 2. Target pos: 2. AABB starts at 2.5.
    // Player rad 1, means player hits AABB at 2.5 - 1 = 1.5.
    // So movement past 1.5 in x should be blocked.
    // Actually, ResolveMovement uses target position, tests collision. 
    // Player moving by 2.0 would have center at 2.0. 
    // AABB min x is 2.5. Distance is 0.5. Player radius is 1.0. Collision!
    // Normal is {1, 0, 0} (from player to AABB). Slide normal is {-1, 0, 0}.
    // CalculateSlideVector({2, 0, 0}, {-1, 0, 0}) -> {0, 0, 0}.
    
    assert(std::abs(resolved.x) < 0.0001f);
    assert(std::abs(resolved.y) < 0.0001f);
    assert(std::abs(resolved.z) < 0.0001f);

    std::cout << "TestResolveMovement passed!" << std::endl;
}

int main() {
    TestVector3();
    TestCylinderCylinder();
    TestCylinderAABB();
    TestSlideVector();
    TestResolveMovement();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
