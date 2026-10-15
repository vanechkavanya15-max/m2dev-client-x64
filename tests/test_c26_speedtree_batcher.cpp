
#include "doctest.h"
#include <vector>
#include <cstdint>

// Simplified mock types
struct D3DVECTOR { float x, y, z; };
struct D3DXVECTOR3 { float x, y, z; };

typedef D3DXVECTOR3 TPosition;
typedef uint32_t DWORD;
typedef DWORD TDiffuse;
struct TTextureCoordinate { float u, v; };
typedef struct SPDTVertex {
	TPosition position;
	TDiffuse diffuse;
	TTextureCoordinate texCoord;
} TPDTVertex;

D3DXVECTOR3 operator*(const D3DXVECTOR3& v, float s) { return {v.x*s, v.y*s, v.z*s}; }
D3DXVECTOR3 operator+(const D3DXVECTOR3& a, const D3DXVECTOR3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
D3DXVECTOR3 operator-(const D3DXVECTOR3& a, const D3DXVECTOR3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }

// Extracted generation logic for unit test
std::vector<TPDTVertex> GenerateQuad(const D3DVECTOR& pos, float width, float height, const D3DXVECTOR3& right, const D3DXVECTOR3& up)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    D3DXVECTOR3 rightScaled = right * halfWidth;
    D3DXVECTOR3 upScaled = up * halfHeight;
    D3DXVECTOR3 center{pos.x, pos.y, pos.z};

    D3DXVECTOR3 v0 = center - rightScaled - upScaled;
    D3DXVECTOR3 v1 = center + rightScaled - upScaled;
    D3DXVECTOR3 v2 = center - rightScaled + upScaled;
    D3DXVECTOR3 v3 = center + rightScaled + upScaled;

    DWORD color = 0xFFFFFFFF;
    TPDTVertex tv0{ {v0.x, v0.y, v0.z}, color, {0.0f, 1.0f} };
    TPDTVertex tv1{ {v1.x, v1.y, v1.z}, color, {1.0f, 1.0f} };
    TPDTVertex tv2{ {v2.x, v2.y, v2.z}, color, {0.0f, 0.0f} };
    TPDTVertex tv3{ {v3.x, v3.y, v3.z}, color, {1.0f, 0.0f} };

    return { tv0, tv2, tv1, tv1, tv2, tv3 };
}

TEST_CASE("SpeedTreeBillboardBatcher Vertex Generation aligned to Camera Right and Up")
{
    // Mock view matrix right and up
    D3DXVECTOR3 right{1.0f, 0.0f, 0.0f}; // Camera looks down Z, right is X
    D3DXVECTOR3 up{0.0f, 1.0f, 0.0f};    // Camera looks down Z, up is Y

    D3DVECTOR pos{10.0f, 20.0f, 30.0f};
    float width = 2.0f; // half = 1.0f
    float height = 4.0f; // half = 2.0f

    auto vertices = GenerateQuad(pos, width, height, right, up);
    REQUIRE(vertices.size() == 6);

    // v0 (bottom-left): center - rightScaled - upScaled
    // 10 - 1 - 0, 20 - 0 - 2, 30 - 0 - 0 => (9, 18, 30)
    CHECK(vertices[0].position.x == doctest::Approx(9.0f));
    CHECK(vertices[0].position.y == doctest::Approx(18.0f));
    CHECK(vertices[0].position.z == doctest::Approx(30.0f));
    CHECK(vertices[0].texCoord.u == doctest::Approx(0.0f));
    CHECK(vertices[0].texCoord.v == doctest::Approx(1.0f));

    // v2 (top-left): center - rightScaled + upScaled
    // 10 - 1 + 0, 20 - 0 + 2, 30 - 0 + 0 => (9, 22, 30)
    CHECK(vertices[1].position.x == doctest::Approx(9.0f));
    CHECK(vertices[1].position.y == doctest::Approx(22.0f));
    CHECK(vertices[1].position.z == doctest::Approx(30.0f));
    CHECK(vertices[1].texCoord.u == doctest::Approx(0.0f));
    CHECK(vertices[1].texCoord.v == doctest::Approx(0.0f));

    // v1 (bottom-right): center + rightScaled - upScaled
    // 10 + 1 - 0, 20 + 0 - 2, 30 + 0 - 0 => (11, 18, 30)
    CHECK(vertices[2].position.x == doctest::Approx(11.0f));
    CHECK(vertices[2].position.y == doctest::Approx(18.0f));
    CHECK(vertices[2].position.z == doctest::Approx(30.0f));
    CHECK(vertices[2].texCoord.u == doctest::Approx(1.0f));
    CHECK(vertices[2].texCoord.v == doctest::Approx(1.0f));

    // v3 (top-right): center + rightScaled + upScaled
    // 10 + 1 + 0, 20 + 0 + 2, 30 + 0 + 0 => (11, 22, 30)
    CHECK(vertices[5].position.x == doctest::Approx(11.0f));
    CHECK(vertices[5].position.y == doctest::Approx(22.0f));
    CHECK(vertices[5].position.z == doctest::Approx(30.0f));
    CHECK(vertices[5].texCoord.u == doctest::Approx(1.0f));
    CHECK(vertices[5].texCoord.v == doctest::Approx(0.0f));
}

