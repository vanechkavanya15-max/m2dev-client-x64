#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define MOCK_TESTING 1
#ifndef D3DMATRIX_DEFINED
#define D3DMATRIX_DEFINED
struct D3DMATRIX {
    float m[4][4];
};
#endif

#include "../src/EterLib/Render/GrannyBoneMatrixCache.h"
#include "../src/EterLib/Render/GrannyBoneMatrixCache.cpp"

using namespace EterLib::Render;

TEST_CASE("GrannyBoneMatrixCache - Basic Cache Hit and Miss")
{
    GrannyBoneMatrixCache cache(100);

    SUBCASE("Cache Miss for empty cache") {
        std::span<D3DMATRIX> out;
        CHECK(cache.TryGetBones(1, out) == false);
    }

    SUBCASE("Store and retrieve") {
        std::vector<D3DMATRIX> bones(10);
        bones[0].m[0][0] = 1.0f;
        cache.StoreBones(1, bones);

        std::span<D3DMATRIX> out;
        CHECK(cache.TryGetBones(1, out) == true);
        CHECK(out.size() == 10);
        CHECK(out[0].m[0][0] == 1.0f);
    }

    SUBCASE("Overwrite existing key") {
        std::vector<D3DMATRIX> bones1(10);
        bones1[0].m[0][0] = 1.0f;
        cache.StoreBones(1, bones1);

        std::vector<D3DMATRIX> bones2(5);
        bones2[0].m[0][0] = 2.0f;
        cache.StoreBones(1, bones2);

        std::span<D3DMATRIX> out;
        CHECK(cache.TryGetBones(1, out) == true);
        CHECK(out.size() == 5);
        CHECK(out[0].m[0][0] == 2.0f);
    }
}

TEST_CASE("GrannyBoneMatrixCache - Capacity and Eviction")
{
    GrannyBoneMatrixCache cache(15); // capacity for 15 bones

    std::vector<D3DMATRIX> bones5(5);
    std::vector<D3DMATRIX> bones10(10);

    SUBCASE("Store up to capacity") {
        cache.StoreBones(1, bones5);
        cache.StoreBones(2, bones10);

        std::span<D3DMATRIX> out;
        CHECK(cache.TryGetBones(1, out) == true);
        CHECK(cache.TryGetBones(2, out) == true);
    }

    SUBCASE("Evict least recently used") {
        cache.StoreBones(1, bones5); // total 5
        cache.StoreBones(2, bones10); // total 15

        // Access 1 so 2 becomes LRU
        std::span<D3DMATRIX> out;
        cache.TryGetBones(1, out);

        std::vector<D3DMATRIX> bones2(2);
        cache.StoreBones(3, bones2); // requires 2, total would be 17. Evicts 2 (size 10), new total 7

        CHECK(cache.TryGetBones(2, out) == false);
        CHECK(cache.TryGetBones(1, out) == true);
        CHECK(cache.TryGetBones(3, out) == true);
    }
}

TEST_CASE("GrannyBoneMatrixCache - Clear")
{
    GrannyBoneMatrixCache cache(100);

    std::vector<D3DMATRIX> bones(10);
    cache.StoreBones(1, bones);
    cache.StoreBones(2, bones);

    cache.Clear();

    std::span<D3DMATRIX> out;
    CHECK(cache.TryGetBones(1, out) == false);
    CHECK(cache.TryGetBones(2, out) == false);
}

