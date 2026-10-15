#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Define Mocks for D3D types to avoid including D3D9.h and StdAfx.h
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
typedef IDirect3DVertexShader9* LPDIRECT3DVERTEXSHADER9;
typedef IDirect3DPixelShader9* LPDIRECT3DPIXELSHADER9;
typedef unsigned int DWORD;

#define TEST_MODE_DISABLE_STDAFX 1

// We can just include the cpp directly to test everything in isolation
// as long as we have our mocks, or we can compile them together. 
// For doctest isolation where we mock D3D types, including the header and cpp is easiest.
#include "../src/EterLib/Render/ShaderRegistryCache.h"
#include "../src/EterLib/Render/ShaderRegistryCache.cpp"

using namespace EterLib::Render;

TEST_CASE("ShaderRegistryCache - Basic Functionality") 
{
    ShaderRegistryCache cache;

    SUBCASE("GetOrCreateShaderPairId allocates new ID for new pairs") 
    {
        LPDIRECT3DVERTEXSHADER9 vs1 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x1234);
        LPDIRECT3DPIXELSHADER9 ps1 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x5678);
        DWORD fvf1 = 1;

        uint16_t id1 = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        CHECK(id1 == 0); // First ID should be 0

        LPDIRECT3DVERTEXSHADER9 vs2 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x1235);
        LPDIRECT3DPIXELSHADER9 ps2 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x5679);
        DWORD fvf2 = 2;

        uint16_t id2 = cache.GetOrCreateShaderPairId(vs2, ps2, fvf2);
        CHECK(id2 == 1); // Second ID should be 1
    }

    SUBCASE("GetOrCreateShaderPairId returns same ID for identical pairs") 
    {
        LPDIRECT3DVERTEXSHADER9 vs1 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x1000);
        LPDIRECT3DPIXELSHADER9 ps1 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x2000);
        DWORD fvf1 = 4;

        uint16_t id1 = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        CHECK(id1 == 0);

        uint16_t id2 = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        CHECK(id2 == 0);
    }

    SUBCASE("GetBindingById retrieves the correct binding") 
    {
        LPDIRECT3DVERTEXSHADER9 vs1 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x1111);
        LPDIRECT3DPIXELSHADER9 ps1 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x2222);
        DWORD fvf1 = 8;

        uint16_t id = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        
        ShaderBinding binding = cache.GetBindingById(id);
        CHECK(binding.vs == vs1);
        CHECK(binding.ps == ps1);
        CHECK(binding.fvf == fvf1);
    }

    SUBCASE("GetBindingById with invalid ID returns empty binding") 
    {
        ShaderBinding binding = cache.GetBindingById(999);
        CHECK(binding.vs == nullptr);
        CHECK(binding.ps == nullptr);
        CHECK(binding.fvf == 0);
    }

    SUBCASE("Clear resets the cache") 
    {
        LPDIRECT3DVERTEXSHADER9 vs1 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x3333);
        LPDIRECT3DPIXELSHADER9 ps1 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x4444);
        DWORD fvf1 = 16;

        uint16_t id1 = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        CHECK(id1 == 0);

        cache.Clear();

        // After clear, registering the same pair should yield ID 0 again
        uint16_t id2 = cache.GetOrCreateShaderPairId(vs1, ps1, fvf1);
        CHECK(id2 == 0);
        
        // Ensure invalid lookup works correctly after clear
        ShaderBinding binding = cache.GetBindingById(1);
        CHECK(binding.vs == nullptr);
    }
}

TEST_CASE("ShaderRegistryCache - Limits and Stress") 
{
    ShaderRegistryCache cache;

    SUBCASE("Registering up to 4096 unique combinations") 
    {
        for (int i = 0; i <= 4095; ++i) 
        {
            LPDIRECT3DVERTEXSHADER9 vs = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(static_cast<uintptr_t>(i + 1));
            LPDIRECT3DPIXELSHADER9 ps = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(static_cast<uintptr_t>(i + 1));
            
            uint16_t id = cache.GetOrCreateShaderPairId(vs, ps, 0);
            CHECK(id == i);
        }

        // Now test exceeding the limit of 4095 IDs
        LPDIRECT3DVERTEXSHADER9 vs_overflow = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0xFFFF);
        LPDIRECT3DPIXELSHADER9 ps_overflow = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0xFFFF);
        
        uint16_t id_overflow = cache.GetOrCreateShaderPairId(vs_overflow, ps_overflow, 0);
        
        // Based on implementation, it should cap out and return 0
        CHECK(id_overflow == 0); 
    }

    SUBCASE("Verify hashing differentiates on FVF") 
    {
        LPDIRECT3DVERTEXSHADER9 vs = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x123);
        LPDIRECT3DPIXELSHADER9 ps = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x456);
        
        uint16_t id1 = cache.GetOrCreateShaderPairId(vs, ps, 1);
        uint16_t id2 = cache.GetOrCreateShaderPairId(vs, ps, 2);
        
        CHECK(id1 == 0);
        CHECK(id2 == 1);
        
        ShaderBinding b1 = cache.GetBindingById(id1);
        ShaderBinding b2 = cache.GetBindingById(id2);
        
        CHECK(b1.fvf == 1);
        CHECK(b2.fvf == 2);
    }
    
    SUBCASE("Verify hashing differentiates on Pixel Shader") 
    {
        LPDIRECT3DVERTEXSHADER9 vs = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x123);
        LPDIRECT3DPIXELSHADER9 ps1 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x456);
        LPDIRECT3DPIXELSHADER9 ps2 = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x789);
        
        uint16_t id1 = cache.GetOrCreateShaderPairId(vs, ps1, 1);
        uint16_t id2 = cache.GetOrCreateShaderPairId(vs, ps2, 1);
        
        CHECK(id1 == 0);
        CHECK(id2 == 1);
    }
    
    SUBCASE("Verify hashing differentiates on Vertex Shader") 
    {
        LPDIRECT3DVERTEXSHADER9 vs1 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x111);
        LPDIRECT3DVERTEXSHADER9 vs2 = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0x222);
        LPDIRECT3DPIXELSHADER9 ps = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0x333);
        
        uint16_t id1 = cache.GetOrCreateShaderPairId(vs1, ps, 1);
        uint16_t id2 = cache.GetOrCreateShaderPairId(vs2, ps, 1);
        
        CHECK(id1 == 0);
        CHECK(id2 == 1);
    }
}

TEST_CASE("ShaderRegistryCache - Additional Edge Cases")
{
    ShaderRegistryCache cache;

    SUBCASE("Handling completely null inputs")
    {
        uint16_t id1 = cache.GetOrCreateShaderPairId(nullptr, nullptr, 0);
        CHECK(id1 == 0);

        ShaderBinding binding = cache.GetBindingById(id1);
        CHECK(binding.vs == nullptr);
        CHECK(binding.ps == nullptr);
        CHECK(binding.fvf == 0);
        
        // Re-fetching same null pair should yield 0
        uint16_t id2 = cache.GetOrCreateShaderPairId(nullptr, nullptr, 0);
        CHECK(id2 == 0);
    }
    
    SUBCASE("Multiple interleaved identical registrations") 
    {
        LPDIRECT3DVERTEXSHADER9 vsA = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0xAAA);
        LPDIRECT3DPIXELSHADER9 psA = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0xBBB);
        DWORD fvfA = 10;
        
        LPDIRECT3DVERTEXSHADER9 vsB = reinterpret_cast<LPDIRECT3DVERTEXSHADER9>(0xCCC);
        LPDIRECT3DPIXELSHADER9 psB = reinterpret_cast<LPDIRECT3DPIXELSHADER9>(0xDDD);
        DWORD fvfB = 20;

        uint16_t id1 = cache.GetOrCreateShaderPairId(vsA, psA, fvfA);
        uint16_t id2 = cache.GetOrCreateShaderPairId(vsB, psB, fvfB);
        uint16_t id3 = cache.GetOrCreateShaderPairId(vsA, psA, fvfA);
        uint16_t id4 = cache.GetOrCreateShaderPairId(vsB, psB, fvfB);
        
        CHECK(id1 == 0);
        CHECK(id2 == 1);
        CHECK(id3 == 0);
        CHECK(id4 == 1);
    }
}

