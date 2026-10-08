#include <cassert>
#include <iostream>
#include <array>

// Mocking D3DVECTOR and D3DPLANE for testing in Linux sandbox
struct D3DVECTOR {
    float x, y, z;
};

struct D3DPLANE {
    float a, b, c, d;
};

#include "../src/EterLib/Render/ActorBoundingFrustum.cpp"

int main()
{
    EterLib::Render::ActorBoundingFrustum frustumCuller;

    // A simple frustum (axis aligned for easy testing)
    // Looking down the +z axis
    std::array<D3DPLANE, 6> frustum = {{
        { 1.0f,  0.0f,  0.0f, 10.0f}, // Left plane
        {-1.0f,  0.0f,  0.0f, 10.0f}, // Right plane
        { 0.0f,  1.0f,  0.0f, 10.0f}, // Bottom plane
        { 0.0f, -1.0f,  0.0f, 10.0f}, // Top plane
        { 0.0f,  0.0f,  1.0f,  0.0f}, // Near plane
        { 0.0f,  0.0f, -1.0f, 100.0f} // Far plane
    }};

    // Test IsActorVisible (Sphere)
    {
        D3DVECTOR pos_inside = {0.0f, 0.0f, 50.0f};
        assert(frustumCuller.IsActorVisible(pos_inside, 1.0f, frustum));

        D3DVECTOR pos_outside = {0.0f, 0.0f, -10.0f}; // Behind near plane
        assert(!frustumCuller.IsActorVisible(pos_outside, 1.0f, frustum));

        D3DVECTOR pos_intersecting = {0.0f, 0.0f, -0.5f};
        assert(frustumCuller.IsActorVisible(pos_intersecting, 1.0f, frustum)); // Radius 1, distance -0.5, so -0.5 >= -1.0 -> true
        
        std::cout << "IsActorVisible tests passed." << std::endl;
    }

    // Test IsBoxVisible (AABB)
    {
        D3DVECTOR min_inside = {-2.0f, -2.0f, 40.0f};
        D3DVECTOR max_inside = { 2.0f,  2.0f, 60.0f};
        assert(frustumCuller.IsBoxVisible(min_inside, max_inside, frustum));

        D3DVECTOR min_outside = {20.0f, -2.0f, 40.0f};
        D3DVECTOR max_outside = {30.0f,  2.0f, 60.0f};
        assert(!frustumCuller.IsBoxVisible(min_outside, max_outside, frustum));

        std::cout << "IsBoxVisible tests passed." << std::endl;
    }

    std::cout << "All ActorBoundingFrustum tests passed successfully." << std::endl;
    return 0;
}

