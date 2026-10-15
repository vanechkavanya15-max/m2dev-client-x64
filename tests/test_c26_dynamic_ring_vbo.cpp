#include "doctest.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// Prevent real d3d9.h from loading on MSVC
#define _D3D9_H_
#define __D3D9_H__

// Mock definitions for Windows/DirectX types
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long HRESULT;

#define S_OK 0
#define E_FAIL -1
#define FAILED(hr) (((HRESULT)(hr)) < 0)

#define D3DUSAGE_DYNAMIC 0x00000200L
#define D3DUSAGE_WRITEONLY 0x00000008L
#define D3DPOOL_DEFAULT 0
#define D3DLOCK_DISCARD 0x00002000
#define D3DLOCK_NOOVERWRITE 0x00001000

// Mock IDirect3DVertexBuffer9
struct IDirect3DVertexBuffer9
{
    int refCount = 1;
    size_t size = 0;
    std::vector<uint8_t> data;

    DWORD lastLockFlags = 0;
    UINT lastLockOffset = 0;
    UINT lastLockSize = 0;

    void Release() { refCount--; }

    HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
    {
        lastLockOffset = OffsetToLock;
        lastLockSize = SizeToLock;
        lastLockFlags = Flags;
        
        if (OffsetToLock + SizeToLock > size)
            return E_FAIL;
            
        *ppbData = data.data() + OffsetToLock;
        return S_OK;
    }

    HRESULT Unlock()
    {
        return S_OK;
    }
};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

// Mock IDirect3DDevice9
struct IDirect3DDevice9
{
    int refCount = 1;

    void AddRef() { refCount++; }
    void Release() { refCount--; }

    HRESULT CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, DWORD Pool, LPDIRECT3DVERTEXBUFFER9* ppVertexBuffer, void** ppSharedHandle)
    {
        *ppVertexBuffer = new IDirect3DVertexBuffer9();
        (*ppVertexBuffer)->size = Length;
        (*ppVertexBuffer)->data.resize(Length);
        return S_OK;
    }
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

// Mock EterBase/Stl.h
#define __INC_ETERBASE_STL_H__
template <typename T>
void safe_release(T& p)
{
    if (p) { p->Release(); p = nullptr; }
}

// Mock EterBase/LogModern.h
#define __INC_ETERBASE_LOGMODERN_H__
// Prevent real LogModern.h from being processed by redefining its guard if any
#define _ETERBASE_LOGMODERN_H_
#define __ETERBASE_LOGMODERN_H__
// Let's not redefine it here and instead mock only what's missing or rely on the real one since it's just a logger and we compile with C++23.
// Actually, looking at LogModern.h, it requires windows.h for OutputDebugStringA on _WIN32. 
// Since we are compiling on Linux for testing, it works out of the box.

// Include implementation directly to test internal logic more easily in isolated environments
#include "../src/EterLib/Render/DynamicRingVertexBuffer.cpp"

using namespace EterLib::Render;

TEST_CASE("DynamicRingVertexBuffer - Initialization and Basic Allocation")
{
    IDirect3DDevice9 device;
    DynamicRingVertexBuffer vbo;

    CHECK(vbo.Initialize(&device, 1024) == true);
    CHECK(vbo.GetBuffer() != nullptr);

    uint8_t data[64] = {0};
    uint32_t offset = vbo.Allocate(64, data, 32);

    CHECK(offset == 0);
    
    // Check lock flags and params
    IDirect3DVertexBuffer9* internalBuffer = vbo.GetBuffer();
    CHECK(internalBuffer->lastLockFlags == D3DLOCK_NOOVERWRITE);
    CHECK(internalBuffer->lastLockOffset == 0);
    CHECK(internalBuffer->lastLockSize == 64);
}

TEST_CASE("DynamicRingVertexBuffer - Wrap-around (D3DLOCK_DISCARD)")
{
    IDirect3DDevice9 device;
    DynamicRingVertexBuffer vbo;

    vbo.Initialize(&device, 100);

    uint8_t data[60] = {0};
    
    // First alloc, fits
    uint32_t offset1 = vbo.Allocate(60, data, 10);
    CHECK(offset1 == 0);
    CHECK(vbo.GetBuffer()->lastLockFlags == D3DLOCK_NOOVERWRITE);

    // Second alloc, does NOT fit in remaining 40 bytes, triggers wrap-around
    uint32_t offset2 = vbo.Allocate(60, data, 10);
    CHECK(offset2 == 0);
    CHECK(vbo.GetBuffer()->lastLockFlags == D3DLOCK_DISCARD);
    
    // Third alloc, fits after wrap
    uint32_t offset3 = vbo.Allocate(30, data, 10);
    CHECK(offset3 == 60);
    CHECK(vbo.GetBuffer()->lastLockFlags == D3DLOCK_NOOVERWRITE);
}

TEST_CASE("DynamicRingVertexBuffer - Offset Alignment")
{
    IDirect3DDevice9 device;
    DynamicRingVertexBuffer vbo;

    vbo.Initialize(&device, 1024);

    uint8_t data1[14] = {0}; // Alloc 14 bytes
    uint8_t data2[32] = {0}; // Next alloc needs 32-byte stride

    uint32_t offset1 = vbo.Allocate(14, data1, 14);
    CHECK(offset1 == 0);

    // Current pointer is at 14. We want to alloc with stride 32.
    // 14 is not aligned to 32. The next aligned offset is 32.
    uint32_t offset2 = vbo.Allocate(32, data2, 32);
    CHECK(offset2 == 32);
}

TEST_CASE("DynamicRingVertexBuffer - Failure Modes")
{
    IDirect3DDevice9 device;
    DynamicRingVertexBuffer vbo;

    CHECK(vbo.Initialize(nullptr, 1024) == false);
    CHECK(vbo.Initialize(&device, 0) == false);

    vbo.Initialize(&device, 100);
    uint8_t data[150] = {0};

    // Allocation larger than entire buffer
    CHECK(vbo.Allocate(150, data, 10) == 0xFFFFFFFF);
    
    // Null data
    CHECK(vbo.Allocate(10, nullptr, 10) == 0xFFFFFFFF);
    
    // Zero size
    CHECK(vbo.Allocate(0, data, 10) == 0xFFFFFFFF);
}

TEST_CASE("DynamicRingVertexBuffer - Reset")
{
    IDirect3DDevice9 device;
    DynamicRingVertexBuffer vbo;

    vbo.Initialize(&device, 1024);
    
    LPDIRECT3DVERTEXBUFFER9 buf = vbo.GetBuffer();
    CHECK(buf != nullptr);
    
    vbo.Reset();
    CHECK(vbo.GetBuffer() == nullptr);
    CHECK(buf->refCount == 0);
}

