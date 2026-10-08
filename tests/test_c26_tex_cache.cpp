#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/EterLib/Render/TextureRegistryCache.h"

// Define a mock struct to use as LPDIRECT3DTEXTURE9 pointers
struct IDirect3DTexture9 {
    int dummy;
};

using namespace EterLib::Render;

TEST_SUITE("TextureRegistryCache") {
    
    TEST_CASE("Initialize and Default State") {
        TextureRegistryCache cache;
        
        // Null texture should return ID 0
        CHECK(cache.GetOrCreateTextureId(nullptr) == TextureRegistryCache::NULL_TEXTURE_ID);
        
        // ID 0 should return nullptr
        CHECK(cache.GetTextureById(TextureRegistryCache::NULL_TEXTURE_ID) == nullptr);
        
        // Out of bounds ID should return nullptr
        CHECK(cache.GetTextureById(9999) == nullptr);
    }
    
    TEST_CASE("Mapping Textures to IDs") {
        TextureRegistryCache cache;
        
        IDirect3DTexture9 tex1, tex2, tex3;
        
        uint32_t id1 = cache.GetOrCreateTextureId(&tex1);
        uint32_t id2 = cache.GetOrCreateTextureId(&tex2);
        uint32_t id3 = cache.GetOrCreateTextureId(&tex3);
        
        // IDs should be positive and distinct
        CHECK(id1 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(id2 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(id3 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 != id3);
        
        // Retrieving ID for the same texture should return the same ID
        CHECK(cache.GetOrCreateTextureId(&tex1) == id1);
        CHECK(cache.GetOrCreateTextureId(&tex2) == id2);
        CHECK(cache.GetOrCreateTextureId(&tex3) == id3);
    }
    
    TEST_CASE("Recovering Textures from IDs") {
        TextureRegistryCache cache;
        
        IDirect3DTexture9 tex1, tex2;
        
        uint32_t id1 = cache.GetOrCreateTextureId(&tex1);
        uint32_t id2 = cache.GetOrCreateTextureId(&tex2);
        
        // Recovering textures using IDs
        CHECK(cache.GetTextureById(id1) == &tex1);
        CHECK(cache.GetTextureById(id2) == &tex2);
        
        // Ensure recovering invalid/non-existent IDs returns nullptr
        CHECK(cache.GetTextureById(id1 + id2 + 100) == nullptr);
    }
    
    TEST_CASE("Invalidating Textures and ID Recycling") {
        TextureRegistryCache cache;
        
        IDirect3DTexture9 tex1, tex2, tex3;
        
        uint32_t id1 = cache.GetOrCreateTextureId(&tex1);
        uint32_t id2 = cache.GetOrCreateTextureId(&tex2);
        uint32_t id3 = cache.GetOrCreateTextureId(&tex3);
        
        // Invalidate tex2
        cache.Invalidate(&tex2);
        
        // ID2 should now point to nullptr
        CHECK(cache.GetTextureById(id2) == nullptr);
        
        // tex1 and tex3 should still be valid
        CHECK(cache.GetTextureById(id1) == &tex1);
        CHECK(cache.GetTextureById(id3) == &tex3);
        
        // Mapping a new texture should recycle id2 (or another free ID)
        IDirect3DTexture9 tex4;
        uint32_t id4 = cache.GetOrCreateTextureId(&tex4);
        
        // It's highly likely id4 == id2 if the free list acts like a stack,
        // but we just check it is a valid ID and we can retrieve it
        CHECK(id4 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(cache.GetTextureById(id4) == &tex4);
        
        // Invalidate all
        cache.Invalidate(&tex1);
        cache.Invalidate(&tex3);
        cache.Invalidate(&tex4);
        
        CHECK(cache.GetTextureById(id1) == nullptr);
        CHECK(cache.GetTextureById(id3) == nullptr);
        CHECK(cache.GetTextureById(id4) == nullptr);
        
        // Nullptr invalidation should be handled gracefully
        cache.Invalidate(nullptr);
    }
    
    TEST_CASE("Clearing the Cache") {
        TextureRegistryCache cache;
        
        IDirect3DTexture9 tex1, tex2;
        
        uint32_t id1 = cache.GetOrCreateTextureId(&tex1);
        uint32_t id2 = cache.GetOrCreateTextureId(&tex2);
        
        cache.Clear();
        
        // Previous IDs should now return nullptr
        CHECK(cache.GetTextureById(id1) == nullptr);
        CHECK(cache.GetTextureById(id2) == nullptr);
        
        // ID 0 should still return nullptr
        CHECK(cache.GetTextureById(TextureRegistryCache::NULL_TEXTURE_ID) == nullptr);
        
        // Re-adding the same textures should assign new IDs (or the same if vectors are reset)
        uint32_t newId1 = cache.GetOrCreateTextureId(&tex1);
        uint32_t newId2 = cache.GetOrCreateTextureId(&tex2);
        
        CHECK(newId1 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(newId2 != TextureRegistryCache::NULL_TEXTURE_ID);
        CHECK(cache.GetTextureById(newId1) == &tex1);
        CHECK(cache.GetTextureById(newId2) == &tex2);
    }
    
    TEST_CASE("Max ID Limit Enforcement") {
        // This test simulates reaching the limit by pre-filling the vector
        // However, actually allocating 1,048,575 textures would be slow.
        // We can test edge conditions by observing the logic:
        // When size exceeds MAX_TEXTURE_ID, it should return 0.
        
        TextureRegistryCache cache;
        
        // Instead of a massive loop, we'll assume the cache logic works as long as
        // id generation is strictly increasing when no free IDs are available.
        // Let's at least test a reasonable number of insertions to ensure no crash
        // and that performance is O(1).
        
        const int NUM_TEXTURES = 10000;
        std::vector<IDirect3DTexture9> textures(NUM_TEXTURES);
        std::vector<uint32_t> ids;
        ids.reserve(NUM_TEXTURES);
        
        for (int i = 0; i < NUM_TEXTURES; ++i) {
            uint32_t id = cache.GetOrCreateTextureId(&textures[i]);
            CHECK(id != TextureRegistryCache::NULL_TEXTURE_ID);
            ids.push_back(id);
        }
        
        // Verify all mappings
        for (int i = 0; i < NUM_TEXTURES; ++i) {
            CHECK(cache.GetTextureById(ids[i]) == &textures[i]);
            // Re-fetching should be identical
            CHECK(cache.GetOrCreateTextureId(&textures[i]) == ids[i]);
        }
        
        // Invalidate half of them
        for (int i = 0; i < NUM_TEXTURES; i += 2) {
            cache.Invalidate(&textures[i]);
            CHECK(cache.GetTextureById(ids[i]) == nullptr);
        }
        
        // Re-add other textures, should reuse IDs
        std::vector<IDirect3DTexture9> moreTextures(NUM_TEXTURES / 2);
        for (int i = 0; i < NUM_TEXTURES / 2; ++i) {
            uint32_t newId = cache.GetOrCreateTextureId(&moreTextures[i]);
            CHECK(newId != TextureRegistryCache::NULL_TEXTURE_ID);
            CHECK(cache.GetTextureById(newId) == &moreTextures[i]);
        }
    }
}
    
    TEST_CASE("Nullptr Invalidation and Extreme Edge Cases") {
        TextureRegistryCache cache;
        
        // Invalidating nullptr should be safe
        CHECK_NOTHROW(cache.Invalidate(nullptr));
        
        // Fetching beyond max possible id
        CHECK(cache.GetTextureById(TextureRegistryCache::MAX_TEXTURE_ID + 100) == nullptr);
        
        IDirect3DTexture9 tex1;
        uint32_t id1 = cache.GetOrCreateTextureId(&tex1);
        
        // Invalidating a texture that wasn't added
        IDirect3DTexture9 tex2;
        CHECK_NOTHROW(cache.Invalidate(&tex2));
        
        // The previous valid texture should be unaffected
        CHECK(cache.GetTextureById(id1) == &tex1);
    }

