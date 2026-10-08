#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#include <vector>
#include <cstring>

#define D3DUSAGE_DYNAMIC 0x00000200L
#define D3DUSAGE_WRITEONLY 0x00000008L
#define D3DPOOL_DEFAULT 0
#define D3DLOCK_DISCARD 0x00002000L
#define D3DSTREAMSOURCE_INDEXEDDATA  0x40000000
#define D3DSTREAMSOURCE_INSTANCEDATA 0x80000000

using UINT = unsigned int;
using DWORD = unsigned long;

#define FAILED(hr) (((long)(hr)) < 0)
#define SUCCEEDED(hr) (((long)(hr)) >= 0)
#define S_OK ((long)0)

// Mock D3DMATRIX for testing purposes
struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
};
typedef struct _D3DMATRIX D3DMATRIX;

struct IDirect3DVertexBuffer9 {
    virtual ~IDirect3DVertexBuffer9() = default;
    virtual long Release() { delete this; return 0; }
    virtual long Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
        *ppbData = buffer.data() + OffsetToLock;
        return S_OK;
    }
    virtual long Unlock() { return S_OK; }
    std::vector<unsigned char> buffer;
};

using LPDIRECT3DVERTEXBUFFER9 = IDirect3DVertexBuffer9*;

struct IDirect3DDevice9 {
    long CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, DWORD Pool, LPDIRECT3DVERTEXBUFFER9* ppVertexBuffer, void** pSharedHandle) {
        auto* vb = new IDirect3DVertexBuffer9();
        vb->buffer.resize(Length);
        *ppVertexBuffer = vb;
        lastCreatedVBLength = Length;
        lastCreatedUsage = Usage;
        return S_OK;
    }
    long SetStreamSourceFreq(UINT StreamNumber, UINT Setting) {
        lastStreamFreqNumber = StreamNumber;
        lastStreamFreqSetting = Setting;
        return S_OK;
    }
    long SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) {
        lastStreamNumber = StreamNumber;
        lastStreamVB = pStreamData;
        lastStreamStride = Stride;
        return S_OK;
    }

    // Mock tracking variables
    UINT lastCreatedVBLength = 0;
    DWORD lastCreatedUsage = 0;
    
    UINT lastStreamNumber = 0;
    LPDIRECT3DVERTEXBUFFER9 lastStreamVB = nullptr;
    UINT lastStreamStride = 0;
    
    UINT lastStreamFreqNumber = 0;
    UINT lastStreamFreqSetting = 0;
};

using LPDIRECT3DDEVICE9 = IDirect3DDevice9*;

// Prevent actual headers from being included and causing redefinition errors
#define _D3D9_H_
#define _D3DX9_H_
#define DIRECT3D_VERSION 0x0900

// Provide a mock HRESULT type which some methods return
typedef long HRESULT;

// We need to define StdAfx.h empty to avoid inclusion issues
// Instead of a real file, we'll just macro it out in this test file
#define STDAFX_H_MOCKED

// Include the source file directly for isolated testing
// To do so we provide a mock of StdAfx.h inline before including it
#include "../src/EterLib/Render/InstanceStreamBuffer.h"

// Provide mock for the cpp file dependencies before including the cpp directly
namespace EterLib::Render {
    // We already have the class definition included above.
}

// Include the cpp file directly
#include "../src/EterLib/Render/InstanceStreamBuffer.cpp"


TEST_CASE("InstanceStreamBuffer Initialization") {
    EterLib::Render::InstanceStreamBuffer buffer;
    IDirect3DDevice9 device;

    // Test failure with null device
    CHECK(buffer.Initialize(nullptr, 100) == false);

    // Test failure with 0 max instances
    CHECK(buffer.Initialize(&device, 0) == false);

    // Test success
    bool result = buffer.Initialize(&device, 100);
    CHECK(result == true);

    // Verify buffer creation parameters
    CHECK(device.lastCreatedVBLength == 100 * sizeof(EterLib::Render::InstanceStreamBuffer::InstanceData));
    CHECK(device.lastCreatedUsage == (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY));
}

TEST_CASE("InstanceStreamBuffer UpdateInstances") {
    EterLib::Render::InstanceStreamBuffer buffer;
    IDirect3DDevice9 device;
    
    buffer.Initialize(&device, 10); // max 10 instances

    std::vector<EterLib::Render::InstanceStreamBuffer::InstanceData> data(5);
    for (int i = 0; i < 5; ++i) {
        data[i].tintColor = (0xFF000000 | i);
        data[i].lodFactor = i * 1.5f;
    }

    buffer.UpdateInstances(data);

    // In our mock, if UpdateInstances works, it copies to the buffer.
    // Let's bind and inspect the buffer through the mocked device.
    buffer.Bind(&device, 1);
    
    REQUIRE(device.lastStreamVB != nullptr);
    auto* vb = device.lastStreamVB;
    REQUIRE(vb->buffer.size() >= 5 * sizeof(EterLib::Render::InstanceStreamBuffer::InstanceData));

    // Reconstruct data from the VB byte buffer
    const auto* copiedData = reinterpret_cast<const EterLib::Render::InstanceStreamBuffer::InstanceData*>(vb->buffer.data());
    for (int i = 0; i < 5; ++i) {
        CHECK(copiedData[i].tintColor == (0xFF000000 | i));
        CHECK(copiedData[i].lodFactor == doctest::Approx(i * 1.5f));
    }
}

TEST_CASE("InstanceStreamBuffer Bind Parameters") {
    EterLib::Render::InstanceStreamBuffer buffer;
    IDirect3DDevice9 device;
    
    buffer.Initialize(&device, 10);
    
    std::vector<EterLib::Render::InstanceStreamBuffer::InstanceData> data(1);
    buffer.UpdateInstances(data);
    
    // Bind to stream 2
    buffer.Bind(&device, 2);

    CHECK(device.lastStreamNumber == 2);
    CHECK(device.lastStreamStride == sizeof(EterLib::Render::InstanceStreamBuffer::InstanceData));
    CHECK(device.lastStreamFreqNumber == 2);
    
    // Validate Frequency divider logic: D3DSTREAMSOURCE_INSTANCEDATA | 1
    UINT expectedSetting = D3DSTREAMSOURCE_INSTANCEDATA | 1;
    CHECK(device.lastStreamFreqSetting == expectedSetting);
}

TEST_CASE("InstanceStreamBuffer Update Bounds Checking") {
    EterLib::Render::InstanceStreamBuffer buffer;
    IDirect3DDevice9 device;
    
    buffer.Initialize(&device, 2); // only max 2 instances
    
    std::vector<EterLib::Render::InstanceStreamBuffer::InstanceData> data(5); // trying to update 5
    for(int i = 0; i < 5; ++i) {
        data[i].tintColor = i;
    }
    
    buffer.UpdateInstances(data);
    buffer.Bind(&device, 1);
    
    auto* vb = device.lastStreamVB;
    REQUIRE(vb != nullptr);
    
    // Verify only 2 were copied by checking that the 3rd element in the buffer (if we try to read past it)
    // Actually we only copied 2, so let's just make sure the first 2 are correct.
    const auto* copiedData = reinterpret_cast<const EterLib::Render::InstanceStreamBuffer::InstanceData*>(vb->buffer.data());
    CHECK(copiedData[0].tintColor == 0);
    CHECK(copiedData[1].tintColor == 1);
    
    // The internal buffer is sized for 2 instances (2 * sizeof(InstanceData))
    CHECK(vb->buffer.size() == 2 * sizeof(EterLib::Render::InstanceStreamBuffer::InstanceData));
}

