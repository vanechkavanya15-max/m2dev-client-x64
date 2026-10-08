#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Define mock structures for testing
// Include the target file
#include "../src/EterLib/Render/MaterialRegistryCache.cpp"

using namespace EterLib::Render;

TEST_SUITE("MaterialRegistryCache") {

    // Helper for creating materials
    D3DMATERIAL9 CreateMaterial(float r, float g, float b, float a) {
        D3DMATERIAL9 mat;
        std::memset(&mat, 0, sizeof(D3DMATERIAL9));
        mat.Diffuse.r = r;
        mat.Diffuse.g = g;
        mat.Diffuse.b = b;
        mat.Diffuse.a = a;
        return mat;
    }

    TEST_CASE("GetOrCreateMaterialId - Add Single Material") {
        MaterialRegistryCache cache;
        
        auto mat = CreateMaterial(1.0f, 0.0f, 0.0f, 1.0f);
        uint8_t id = cache.GetOrCreateMaterialId(mat);
        
        CHECK_EQ(id, 0); // First material should have ID 0
        
        auto retrievedMat = cache.GetMaterialById(id);
        CHECK_EQ(retrievedMat.Diffuse.r, doctest::Approx(1.0f));
        CHECK_EQ(retrievedMat.Diffuse.g, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat.Diffuse.b, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat.Diffuse.a, doctest::Approx(1.0f));
    }
    
    TEST_CASE("GetOrCreateMaterialId - Add Same Material Multiple Times") {
        MaterialRegistryCache cache;
        
        auto mat1 = CreateMaterial(0.5f, 0.5f, 0.5f, 1.0f);
        uint8_t id1 = cache.GetOrCreateMaterialId(mat1);
        
        auto mat2 = CreateMaterial(0.5f, 0.5f, 0.5f, 1.0f); // Identical values
        uint8_t id2 = cache.GetOrCreateMaterialId(mat2);
        
        CHECK_EQ(id1, id2); // Should return the same ID
        CHECK_EQ(id1, 0);   // Should be 0 since it's the first unique material
    }
    
    TEST_CASE("GetOrCreateMaterialId - Add Multiple Unique Materials") {
        MaterialRegistryCache cache;
        
        auto mat1 = CreateMaterial(1.0f, 0.0f, 0.0f, 1.0f);
        auto mat2 = CreateMaterial(0.0f, 1.0f, 0.0f, 1.0f);
        auto mat3 = CreateMaterial(0.0f, 0.0f, 1.0f, 1.0f);
        
        uint8_t id1 = cache.GetOrCreateMaterialId(mat1);
        uint8_t id2 = cache.GetOrCreateMaterialId(mat2);
        uint8_t id3 = cache.GetOrCreateMaterialId(mat3);
        
        CHECK_EQ(id1, 0);
        CHECK_EQ(id2, 1);
        CHECK_EQ(id3, 2);
        
        // Verify retrieved values
        auto retrievedMat2 = cache.GetMaterialById(id2);
        CHECK_EQ(retrievedMat2.Diffuse.r, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat2.Diffuse.g, doctest::Approx(1.0f));
        CHECK_EQ(retrievedMat2.Diffuse.b, doctest::Approx(0.0f));
        
        auto retrievedMat3 = cache.GetMaterialById(id3);
        CHECK_EQ(retrievedMat3.Diffuse.r, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat3.Diffuse.g, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat3.Diffuse.b, doctest::Approx(1.0f));
    }
    
    TEST_CASE("Clear - Clears the cache") {
        MaterialRegistryCache cache;
        
        auto mat = CreateMaterial(1.0f, 1.0f, 1.0f, 1.0f);
        cache.GetOrCreateMaterialId(mat);
        
        cache.Clear();
        
        // After clearing, inserting the same material again should give ID 0
        uint8_t idAfterClear = cache.GetOrCreateMaterialId(mat);
        CHECK_EQ(idAfterClear, 0);
    }
    
    TEST_CASE("GetMaterialById - Invalid ID (empty cache)") {
        MaterialRegistryCache cache;
        
        // Querying an empty cache
        auto retrievedMat = cache.GetMaterialById(99);
        
        // Should return a default initialized material (all zeros)
        CHECK_EQ(retrievedMat.Diffuse.r, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat.Diffuse.g, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat.Diffuse.b, doctest::Approx(0.0f));
        CHECK_EQ(retrievedMat.Diffuse.a, doctest::Approx(0.0f));
    }
    
    TEST_CASE("GetMaterialById - Invalid ID (populated cache)") {
        MaterialRegistryCache cache;
        
        auto mat = CreateMaterial(0.1f, 0.2f, 0.3f, 1.0f);
        cache.GetOrCreateMaterialId(mat);
        
        // Querying an invalid ID
        auto retrievedMat = cache.GetMaterialById(99);
        
        // Should return the first material in cache
        CHECK_EQ(retrievedMat.Diffuse.r, doctest::Approx(0.1f));
        CHECK_EQ(retrievedMat.Diffuse.g, doctest::Approx(0.2f));
        CHECK_EQ(retrievedMat.Diffuse.b, doctest::Approx(0.3f));
        CHECK_EQ(retrievedMat.Diffuse.a, doctest::Approx(1.0f));
    }
    
    TEST_CASE("GetOrCreateMaterialId - Reaching capacity limit") {
        MaterialRegistryCache cache;
        
        // Fill cache with 256 unique materials
        for (int i = 0; i < 256; ++i) {
            float val = static_cast<float>(i) / 255.0f;
            auto mat = CreateMaterial(val, val, val, 1.0f);
            uint8_t id = cache.GetOrCreateMaterialId(mat);
            CHECK_EQ(id, i);
        }
        
        // Add 257th material
        auto matOverflow = CreateMaterial(1.0f, 0.0f, 1.0f, 1.0f);
        uint8_t idOverflow = cache.GetOrCreateMaterialId(matOverflow);
        
        // Should hit limit and return 0
        CHECK_EQ(idOverflow, 0);
        
        // Cache should still only hold 256 materials
        auto retrievedMat = cache.GetMaterialById(255);
        CHECK_EQ(retrievedMat.Diffuse.r, doctest::Approx(1.0f)); // Last added valid material
        
        // 257th material should not be retrievable by ID 0
        auto retrievedZeroMat = cache.GetMaterialById(0);
        CHECK_EQ(retrievedZeroMat.Diffuse.r, doctest::Approx(0.0f)); // First added valid material
    }
    
    TEST_CASE("MaterialHasher and MaterialEqual") {
        MaterialHasher hasher;
        MaterialEqual equals;
        
        auto mat1 = CreateMaterial(0.5f, 0.5f, 0.5f, 1.0f);
        auto mat2 = CreateMaterial(0.5f, 0.5f, 0.5f, 1.0f);
        auto mat3 = CreateMaterial(0.6f, 0.5f, 0.5f, 1.0f);
        
        // Equal materials should have same hash
        CHECK_EQ(hasher(mat1), hasher(mat2));
        
        // Different materials should likely have different hash
        CHECK_NE(hasher(mat1), hasher(mat3));
        
        // Equality check
        CHECK(equals(mat1, mat2));
        CHECK_FALSE(equals(mat1, mat3));
    }
    
    TEST_CASE("MaterialRegistryCache - Comprehensive stress test") {
        MaterialRegistryCache cache;
        
        std::vector<D3DMATERIAL9> testMats;
        
        // Generate 100 random looking materials
        for (int i = 0; i < 100; ++i) {
            float r = static_cast<float>(i % 10) / 10.0f;
            float g = static_cast<float>((i * 2) % 10) / 10.0f;
            float b = static_cast<float>((i * 3) % 10) / 10.0f;
            
            testMats.push_back(CreateMaterial(r, g, b, 1.0f));
        }
        
        // Add them all
        std::vector<uint8_t> ids;
        for (const auto& mat : testMats) {
            ids.push_back(cache.GetOrCreateMaterialId(mat));
        }
        
        // Verify them all
        for (size_t i = 0; i < testMats.size(); ++i) {
            auto retrievedMat = cache.GetMaterialById(ids[i]);
            
            CHECK_EQ(retrievedMat.Diffuse.r, doctest::Approx(testMats[i].Diffuse.r));
            CHECK_EQ(retrievedMat.Diffuse.g, doctest::Approx(testMats[i].Diffuse.g));
            CHECK_EQ(retrievedMat.Diffuse.b, doctest::Approx(testMats[i].Diffuse.b));
            CHECK_EQ(retrievedMat.Diffuse.a, doctest::Approx(testMats[i].Diffuse.a));
        }
        
        // Add some duplicates and verify IDs match original
        for (size_t i = 0; i < 10; ++i) {
            uint8_t duplicateId = cache.GetOrCreateMaterialId(testMats[i]);
            CHECK_EQ(duplicateId, ids[i]);
        }
        
        // Clear and add first one again
        cache.Clear();
        uint8_t newId = cache.GetOrCreateMaterialId(testMats[0]);
        CHECK_EQ(newId, 0);
    }
}

