#define TEST_MODE_DISABLE_STDAFX
#include "../src/EterLib/Render/GeometryBufferRegistry.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <format>

using namespace EterLib::Render;

// Mock implementations
struct MockDevice : public IDirect3DDevice9 {
    int setStreamSourceCalls = 0;
    int setIndicesCalls = 0;

    HRESULT SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) override {
        setStreamSourceCalls++;
        return S_OK;
    }

    HRESULT SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData) override {
        setIndicesCalls++;
        return S_OK;
    }

    void ResetCalls() {
        setStreamSourceCalls = 0;
        setIndicesCalls = 0;
    }
};

void TestVertexBufferBinding_Basic() {
    std::cout << "Running TestVertexBufferBinding_Basic...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vb1;

    // Bind for the first time
    bool result = registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    assert(result);
    assert(device.setStreamSourceCalls == 1);

    // Bind same buffer again, should not increment calls
    result = registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    assert(result);
    assert(device.setStreamSourceCalls == 1);

    std::cout << "TestVertexBufferBinding_Basic passed.\n";
}

void TestVertexBufferBinding_DifferentStreams() {
    std::cout << "Running TestVertexBufferBinding_DifferentStreams...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vb1, vb2;

    // Bind stream 0
    registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    assert(device.setStreamSourceCalls == 1);

    // Bind stream 1
    registry.BindVertexBuffer(&device, 1, &vb2, 0, 32);
    assert(device.setStreamSourceCalls == 2);

    // Re-bind stream 0 with same
    registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    assert(device.setStreamSourceCalls == 2);

    // Re-bind stream 1 with same
    registry.BindVertexBuffer(&device, 1, &vb2, 0, 32);
    assert(device.setStreamSourceCalls == 2);

    std::cout << "TestVertexBufferBinding_DifferentStreams passed.\n";
}

void TestVertexBufferBinding_DifferentProperties() {
    std::cout << "Running TestVertexBufferBinding_DifferentProperties...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vb1, vb2;

    registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    assert(device.setStreamSourceCalls == 1);

    // Different offset
    registry.BindVertexBuffer(&device, 0, &vb1, 4, 16);
    assert(device.setStreamSourceCalls == 2);

    // Different stride
    registry.BindVertexBuffer(&device, 0, &vb1, 4, 32);
    assert(device.setStreamSourceCalls == 3);

    // Different buffer
    registry.BindVertexBuffer(&device, 0, &vb2, 4, 32);
    assert(device.setStreamSourceCalls == 4);

    std::cout << "TestVertexBufferBinding_DifferentProperties passed.\n";
}

void TestVertexBufferBinding_InvalidArguments() {
    std::cout << "Running TestVertexBufferBinding_InvalidArguments...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vb1;

    // Null device
    bool result = registry.BindVertexBuffer(nullptr, 0, &vb1, 0, 16);
    assert(!result);
    assert(device.setStreamSourceCalls == 0);

    // Out of bounds stream
    result = registry.BindVertexBuffer(&device, 16, &vb1, 0, 16);
    assert(!result);
    assert(device.setStreamSourceCalls == 0);

    std::cout << "TestVertexBufferBinding_InvalidArguments passed.\n";
}

void TestIndexBufferBinding_Basic() {
    std::cout << "Running TestIndexBufferBinding_Basic...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DIndexBuffer9 ib1;

    // Bind for the first time
    bool result = registry.BindIndexBuffer(&device, &ib1);
    assert(result);
    assert(device.setIndicesCalls == 1);

    // Bind same buffer again
    result = registry.BindIndexBuffer(&device, &ib1);
    assert(result);
    assert(device.setIndicesCalls == 1);

    std::cout << "TestIndexBufferBinding_Basic passed.\n";
}

void TestIndexBufferBinding_DifferentBuffers() {
    std::cout << "Running TestIndexBufferBinding_DifferentBuffers...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DIndexBuffer9 ib1, ib2;

    registry.BindIndexBuffer(&device, &ib1);
    assert(device.setIndicesCalls == 1);

    registry.BindIndexBuffer(&device, &ib2);
    assert(device.setIndicesCalls == 2);

    registry.BindIndexBuffer(&device, &ib1);
    assert(device.setIndicesCalls == 3);

    std::cout << "TestIndexBufferBinding_DifferentBuffers passed.\n";
}

void TestIndexBufferBinding_InvalidArguments() {
    std::cout << "Running TestIndexBufferBinding_InvalidArguments...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DIndexBuffer9 ib1;

    bool result = registry.BindIndexBuffer(nullptr, &ib1);
    assert(!result);
    assert(device.setIndicesCalls == 0);

    std::cout << "TestIndexBufferBinding_InvalidArguments passed.\n";
}

void TestInvalidate() {
    std::cout << "Running TestInvalidate...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vb1;
    IDirect3DIndexBuffer9 ib1;

    registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    registry.BindIndexBuffer(&device, &ib1);

    assert(device.setStreamSourceCalls == 1);
    assert(device.setIndicesCalls == 1);

    registry.Invalidate();

    // Re-bind after invalidate, should call the device again
    registry.BindVertexBuffer(&device, 0, &vb1, 0, 16);
    registry.BindIndexBuffer(&device, &ib1);

    assert(device.setStreamSourceCalls == 2);
    assert(device.setIndicesCalls == 2);

    std::cout << "TestInvalidate passed.\n";
}

void TestStress() {
    std::cout << "Running TestStress...\n";
    GeometryBufferRegistry registry;
    MockDevice device;
    IDirect3DVertexBuffer9 vbs[16];
    IDirect3DIndexBuffer9 ibs[4];

    for (int i = 0; i < 1000; ++i) {
        int stream = i % 16;
        int bufIdx = (i / 16) % 16;
        registry.BindVertexBuffer(&device, stream, &vbs[bufIdx], i % 8, 16 + (i % 4));
        
        int idxBuf = i % 4;
        registry.BindIndexBuffer(&device, &ibs[idxBuf]);
    }
    
    // We just verify it doesn't crash and works logic wise, actual calls depend on the modulus pattern
    // The main point is testing multiple iterations.
    
    std::cout << "TestStress passed.\n";
}

int main() {
    std::cout << "Starting GeometryBufferRegistry tests...\n";
    
    TestVertexBufferBinding_Basic();
    TestVertexBufferBinding_DifferentStreams();
    TestVertexBufferBinding_DifferentProperties();
    TestVertexBufferBinding_InvalidArguments();
    
    TestIndexBufferBinding_Basic();
    TestIndexBufferBinding_DifferentBuffers();
    TestIndexBufferBinding_InvalidArguments();
    
    TestInvalidate();
    TestStress();
    
    std::cout << "All tests passed successfully.\n";
    return 0;
}

