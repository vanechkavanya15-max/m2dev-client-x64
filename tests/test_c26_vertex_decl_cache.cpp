#include "../src/EterLib/Render/VertexDeclarationCache.h"
#include <cassert>
#include <iostream>

using namespace EterLib::Render;

class MockVertexDeclaration : public IDirect3DVertexDeclaration9
{
public:
    int releaseCount = 0;
    
    ULONG Release() override
    {
        releaseCount++;
        delete this; // fix leak
        return 0;
    }
};

class MockDevice : public IDirect3DDevice9
{
public:
    int createCount = 0;

    HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) override
    {
        createCount++;
        *ppDecl = new MockVertexDeclaration();
        return D3D_OK;
    }
};

void test_empty_cache()
{
    VertexDeclarationCache cache;
    assert(cache.Count() == 0);
}

void test_get_or_create()
{
    VertexDeclarationCache cache;
    MockDevice dev;
    
    D3DVERTEXELEMENT9 elements1[] = {
        {0, 0, 0, 0, 0, 0},
        {0, 12, 1, 0, 1, 0}
    };
    
    auto decl1 = cache.GetOrCreate(&dev, elements1);
    assert(decl1 != nullptr);
    assert(dev.createCount == 1);
    assert(cache.Count() == 1);
    
    auto decl2 = cache.GetOrCreate(&dev, elements1);
    assert(decl2 == decl1); // Should return the same cached instance
    assert(dev.createCount == 1); // Should not create a new one
    assert(cache.Count() == 1);
    
    D3DVERTEXELEMENT9 elements2[] = {
        {0, 0, 0, 0, 0, 0},
        {0, 12, 1, 0, 1, 0},
        {0, 24, 2, 0, 2, 0}
    };
    
    auto decl3 = cache.GetOrCreate(&dev, elements2);
    assert(decl3 != nullptr);
    assert(decl3 != decl1); // Different elements, different decl
    assert(dev.createCount == 2);
    assert(cache.Count() == 2);
}

void test_clear()
{
    VertexDeclarationCache cache;
    MockDevice dev;
    
    D3DVERTEXELEMENT9 elements[] = {
        {0, 0, 0, 0, 0, 0}
    };
    
    auto decl = cache.GetOrCreate(&dev, elements);
    assert(cache.Count() == 1);
    
    cache.Clear();
    
    assert(cache.Count() == 0);
}

int main()
{
    test_empty_cache();
    test_get_or_create();
    test_clear();
    std::cout << "All tests passed!\n";
    return 0;
}

