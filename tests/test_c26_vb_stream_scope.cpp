#include <iostream>
#include <cassert>
#include <stdexcept>
#include <string>

// Prevent real d3d9.h from loading on MSVC to avoid COM massive pure virtual boilerplate
#define _D3D9_H_
#define __D3D9_H__

// Mock definitions for Windows/DirectX types
typedef unsigned int UINT;
typedef long HRESULT;

#define S_OK 0
#define E_FAIL -1
#define FAILED(hr) (((HRESULT)(hr)) < 0)

// Mock DirectX interfaces (simplified for the test)
struct IDirect3DVertexBuffer9
{
    int refCount = 1;
    void Release() { refCount--; }
};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

struct IDirect3DDevice9EX
{
    LPDIRECT3DVERTEXBUFFER9 currentBuffer = nullptr;
    UINT currentOffset = 0;
    UINT currentStride = 0;
    bool getStreamSourceFails = false;
    bool setStreamSourceFails = false;
    
    int setStreamSourceCalls = 0;
    int getStreamSourceCalls = 0;

    HRESULT GetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9* ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
    {
        getStreamSourceCalls++;
        if (getStreamSourceFails) return E_FAIL;
        *ppStreamData = currentBuffer;
        *pOffsetInBytes = currentOffset;
        *pStride = currentStride;
        if (currentBuffer) currentBuffer->refCount++;
        return S_OK;
    }

    HRESULT SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride)
    {
        setStreamSourceCalls++;
        if (setStreamSourceFails) return E_FAIL;
        currentBuffer = pStreamData;
        currentOffset = OffsetInBytes;
        currentStride = Stride;
        return S_OK;
    }
};
typedef IDirect3DDevice9EX* LPDIRECT3DDEVICE9EX;

// Mock EterBase/Stl.h
#define __INC_ETERBASE_STL_H__
template <typename T>
void safe_release(T& p)
{
    if (p) { p->Release(); p = nullptr; }
}

// Mock EterBase/LogModern.h
#define __INC_ETERBASE_LOGMODERN_H__
namespace EterBase {
    class ModernLogger {
    public:
        static void Error(const std::string& msg) { }
    };
}

#include "EterLib/Render/VertexBufferStreamScope.h"

void TestNormalOperation()
{
    IDirect3DDevice9EX device;
    IDirect3DVertexBuffer9 oldBuffer;
    IDirect3DVertexBuffer9 newBuffer;
    
    device.currentBuffer = &oldBuffer;
    device.currentOffset = 10;
    device.currentStride = 32;
    oldBuffer.refCount = 1;
    newBuffer.refCount = 1;
    
    {
        EterLib::Render::VertexBufferStreamScope scope(&device, 0, &newBuffer, 0, 64);
        assert(device.getStreamSourceCalls == 1);
        assert(device.setStreamSourceCalls == 1);
        assert(device.currentBuffer == &newBuffer);
        assert(device.currentOffset == 0);
        assert(device.currentStride == 64);
        assert(oldBuffer.refCount == 2);
        assert(scope.IsActive());
    }
    
    assert(device.setStreamSourceCalls == 2);
    assert(device.currentBuffer == &oldBuffer);
    assert(device.currentOffset == 10);
    assert(device.currentStride == 32);
    assert(oldBuffer.refCount == 1);
}

void TestInvalidArguments()
{
    IDirect3DDevice9EX device;
    IDirect3DVertexBuffer9 newBuffer;
    
    try
    {
        EterLib::Render::VertexBufferStreamScope scope(nullptr, 0, &newBuffer, 0, 32);
        assert(false);
    }
    catch (const std::invalid_argument&) {}
    
    try
    {
        EterLib::Render::VertexBufferStreamScope scope(&device, 16, &newBuffer, 0, 32);
        assert(false);
    }
    catch (const std::invalid_argument&) {}
}

void TestGetStreamSourceFailure()
{
    IDirect3DDevice9EX device;
    IDirect3DVertexBuffer9 newBuffer;
    device.getStreamSourceFails = true;
    
    {
        EterLib::Render::VertexBufferStreamScope scope(&device, 0, &newBuffer, 0, 64);
        assert(scope.IsActive());
        assert(device.setStreamSourceCalls == 1);
    }
    assert(device.setStreamSourceCalls == 2);
    assert(device.currentBuffer == nullptr);
}

void TestSetStreamSourceFailure()
{
    IDirect3DDevice9EX device;
    IDirect3DVertexBuffer9 oldBuffer;
    IDirect3DVertexBuffer9 newBuffer;
    device.currentBuffer = &oldBuffer;
    device.currentOffset = 10;
    device.currentStride = 32;
    device.setStreamSourceFails = true;
    
    {
        EterLib::Render::VertexBufferStreamScope scope(&device, 0, &newBuffer, 0, 64);
        assert(!scope.IsActive());
        assert(device.setStreamSourceCalls == 1);
    }
    assert(device.setStreamSourceCalls == 1); // Should not call set again on restore
    assert(oldBuffer.refCount == 1); // safe_release must have been called
}

void TestMoveSemantics()
{
    IDirect3DDevice9EX device;
    IDirect3DVertexBuffer9 oldBuffer;
    IDirect3DVertexBuffer9 newBuffer;
    device.currentBuffer = &oldBuffer;
    
    {
        EterLib::Render::VertexBufferStreamScope scope1(&device, 0, &newBuffer, 0, 32);
        EterLib::Render::VertexBufferStreamScope scope2(std::move(scope1));
        assert(!scope1.IsActive());
        assert(scope2.IsActive());
        assert(device.setStreamSourceCalls == 1);
    }
    assert(device.setStreamSourceCalls == 2);
    assert(device.currentBuffer == &oldBuffer);
    assert(oldBuffer.refCount == 1);
}

int main()
{
    try
    {
        TestNormalOperation();
        TestInvalidArguments();
        TestGetStreamSourceFailure();
        TestSetStreamSourceFailure();
        TestMoveSemantics();
        std::cout << "All tests passed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
