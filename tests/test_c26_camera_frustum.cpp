// tests/test_c26_camera_frustum.cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// In the Linux test environment, we mock the required DirectX 9 Math types
// directly in the test file (as per memory instructions) rather than polluting
// the workspace with fake `d3dx9.h` headers.
#ifndef _WIN32
#include <cmath>

struct D3DXVECTOR3 {
    float x, y, z;
    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct D3DXPLANE {
    float a, b, c, d;
    D3DXPLANE() : a(0), b(0), c(0), d(0) {}
    D3DXPLANE(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}
};

struct D3DXMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
    D3DXMATRIX() {}
};

inline D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP) {
    float norm = std::sqrt(pP->a * pP->a + pP->b * pP->b + pP->c * pP->c);
    if (norm != 0.0f) {
        pOut->a = pP->a / norm;
        pOut->b = pP->b / norm;
        pOut->c = pP->c / norm;
        pOut->d = pP->d / norm;
    }
    return pOut;
}

inline float D3DXPlaneDotCoord(const D3DXPLANE* pP, const D3DXVECTOR3* pV) {
    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

// In Linux Sandbox we bypass standard windows headers
#define _INC_D3DX9
#define _D3D9_H_
#define _WINDOWS_
#endif

// To actually test the real code without duplicating it in the test file, 
// we include the header and compile it against the real CPP file.
#include "../src/Client/Graphics/CameraFrustum.h"

// If we are in linux test environment and testing directly, we can include the cpp to compile it here.
#ifndef _WIN32
#include "../src/Client/Graphics/CameraFrustum.cpp"
#endif

using namespace Client::Graphics;

TEST_CASE("CameraFrustum Culling tests") {
    CameraFrustum frustum;
    D3DXMATRIX viewProj;
    
    // Orthographic projection matrix simulation
    // Bounds: Left=-5, Right=5, Bottom=-5, Top=5, Near=1, Far=100
    viewProj._11 = 1.0f / 5.0f; viewProj._12 = 0.0f; viewProj._13 = 0.0f; viewProj._14 = 0.0f;
    viewProj._21 = 0.0f; viewProj._22 = 1.0f / 5.0f; viewProj._23 = 0.0f; viewProj._24 = 0.0f;
    viewProj._31 = 0.0f; viewProj._32 = 0.0f; viewProj._33 = 1.0f / (100.0f - 1.0f); viewProj._34 = 0.0f;
    viewProj._41 = 0.0f; viewProj._42 = 0.0f; viewProj._43 = -1.0f / (100.0f - 1.0f); viewProj._44 = 1.0f;

    frustum.Update(viewProj);

    SUBCASE("IsPointVisible evaluates correctly") {
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(0.0f, 0.0f, 50.0f)) == true); // Inside
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(6.0f, 0.0f, 50.0f)) == false); // Outside Right
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(0.0f, 6.0f, 50.0f)) == false); // Outside Top
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(0.0f, 0.0f, 150.0f)) == false); // Outside Far
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(0.0f, 0.0f, 0.0f)) == false); // Outside Near
    }

    SUBCASE("IsPointVisible uses safe culling margin") {
        // Point is at X=6, strictly outside right plane (X=5)
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(6.0f, 0.0f, 50.0f), 0.5f) == false); // Margin 0.5 not enough
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(6.0f, 0.0f, 50.0f), 1.0f) == true);  // Margin 1.0 overlaps boundary
        CHECK(frustum.IsPointVisible(D3DXVECTOR3(6.0f, 0.0f, 50.0f), 2.0f) == true);  // Margin 2.0 safely covers it
    }

    SUBCASE("IsSphereVisible evaluates correctly") {
        CHECK(frustum.IsSphereVisible(D3DXVECTOR3(0.0f, 0.0f, 50.0f), 1.0f) == true); // Fully inside
        CHECK(frustum.IsSphereVisible(D3DXVECTOR3(5.5f, 0.0f, 50.0f), 1.0f) == true); // Intersects Right boundary
        CHECK(frustum.IsSphereVisible(D3DXVECTOR3(6.5f, 0.0f, 50.0f), 1.0f) == false); // Fully outside
    }

    SUBCASE("IsSphereVisible uses safe culling margin") {
        // Sphere is outside, but margin saves it (e.g. boss/archer logic)
        CHECK(frustum.IsSphereVisible(D3DXVECTOR3(7.0f, 0.0f, 50.0f), 1.0f) == false); // Fails (radius reaches X=6, boundary at 5)
        CHECK(frustum.IsSphereVisible(D3DXVECTOR3(7.0f, 0.0f, 50.0f), 1.0f, 1.0f) == true); // Succeeds with margin 1.0
    }

    SUBCASE("IsAABBVisible evaluates correctly") {
        CHECK(frustum.IsAABBVisible(D3DXVECTOR3(-1.0f, -1.0f, 49.0f), D3DXVECTOR3(1.0f, 1.0f, 51.0f)) == true); // Inside
        CHECK(frustum.IsAABBVisible(D3DXVECTOR3(5.1f, -1.0f, 49.0f), D3DXVECTOR3(7.1f, 1.0f, 51.0f)) == false); // Outside Right
        CHECK(frustum.IsAABBVisible(D3DXVECTOR3(4.9f, -1.0f, 49.0f), D3DXVECTOR3(6.9f, 1.0f, 51.0f)) == true); // Intersects Right
    }
    
    SUBCASE("IsAABBVisible uses safe culling margin") {
        // AABB strictly outside Right plane (X > 5)
        CHECK(frustum.IsAABBVisible(D3DXVECTOR3(5.5f, -1.0f, 49.0f), D3DXVECTOR3(7.5f, 1.0f, 51.0f)) == false); // Without margin
        CHECK(frustum.IsAABBVisible(D3DXVECTOR3(5.5f, -1.0f, 49.0f), D3DXVECTOR3(7.5f, 1.0f, 51.0f), 0.5f) == true); // With margin 0.5 (extends bound to 5)
    }
}
