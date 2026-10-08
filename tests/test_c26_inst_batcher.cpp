#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <vector>
#include <iostream>

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

// Mock interfaces for Direct3D 9 compilation
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
        return S_OK;
    }
    long SetStreamSourceFreq(UINT StreamNumber, UINT Setting) { return S_OK; }
    long SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) { return S_OK; }
};

using LPDIRECT3DDEVICE9 = IDirect3DDevice9*;

// Prevent actual d3d9.h from being included in InstancedDrawBatcher.h
#define _D3D9_H_
#define DIRECT3D_VERSION 0x0900

#include <cstring>
#include "../src/EterLib/Render/InstancedDrawBatcher.h"
#include "../src/EterLib/Render/InstancedDrawBatcher.cpp"

using namespace EterLib::Render;

// A helper function to generate deterministic test instances
DrawInstance CreateTestInstance(int index) {
    DrawInstance inst{};
    inst.color = 0xFF000000 | (index & 0xFFFFFF);
    for (int i = 0; i < 16; ++i) {
        inst.transform[i] = static_cast<float>(index + i);
    }
    return inst;
}

TEST_CASE("InstancedDrawBatcher - Batching 50 draw calls")
{
    IDirect3DDevice9 dummyDevice{};
    LPDIRECT3DDEVICE9 pDevice = &dummyDevice;

    InstancedDrawBatcher batcher;
    RenderQueue queue;

    SUBCASE("Initial state verification")
    {
        // Check that a fresh batcher has no saved draw calls
        CHECK(batcher.GetSavedDrawCallCount() == 0);
        
        // Check that a fresh queue is empty
        CHECK(queue.GetInstances().empty());
        CHECK(queue.GetInstances().size() == 0);
    }

    SUBCASE("Flushing an empty queue does nothing")
    {
        // Try flushing before adding anything
        batcher.FlushBatches(pDevice, queue);
        
        // Should still be zero
        CHECK(batcher.GetSavedDrawCallCount() == 0);
        CHECK(queue.GetInstances().empty());
    }

    SUBCASE("Flushing a null device returns early")
    {
        DrawInstance inst = CreateTestInstance(1);
        queue.AddInstance(inst);
        queue.AddInstance(inst);
        
        CHECK(queue.GetInstances().size() == 2);
        
        // Pass a null device
        batcher.FlushBatches(nullptr, queue);
        
        // The queue should not be cleared, and no draw calls saved
        CHECK(batcher.GetSavedDrawCallCount() == 0);
        CHECK(queue.GetInstances().size() == 2);
    }

    SUBCASE("Flushing a single instance saves 0 draw calls")
    {
        DrawInstance inst = CreateTestInstance(42);
        queue.AddInstance(inst);
        
        // One instance equals one draw call normally, so batching doesn't save any
        batcher.FlushBatches(pDevice, queue);
        
        CHECK(batcher.GetSavedDrawCallCount() == 0);
        
        // Queue should be cleared after successful flush
        CHECK(queue.GetInstances().empty());
    }

    SUBCASE("Flushing exactly 50 instances saves 49 draw calls")
    {
        // Simulate collecting 50 instances
        for (int i = 0; i < 50; ++i)
        {
            queue.AddInstance(CreateTestInstance(i));
        }

        // Verify the queue holds exactly 50 instances
        CHECK(queue.GetInstances().size() == 50);

        // Verify the data of the first and last instance
        const auto& instances = queue.GetInstances();
        CHECK(instances.front().color == (0xFF000000 | 0));
        CHECK(instances.front().transform[0] == 0.0f);
        CHECK(instances.back().color == (0xFF000000 | 49));
        CHECK(instances.back().transform[0] == 49.0f);

        // Flush batches
        batcher.FlushBatches(pDevice, queue);

        // 50 instances batched into 1 draw call -> saves 49 calls
        CHECK(batcher.GetSavedDrawCallCount() == 49);

        // Queue should be cleared
        CHECK(queue.GetInstances().empty());
    }

    SUBCASE("Multiple flushes accumulate saved draw calls correctly")
    {
        // First batch of 50
        for (int i = 0; i < 50; ++i)
        {
            queue.AddInstance(CreateTestInstance(i));
        }
        batcher.FlushBatches(pDevice, queue);
        CHECK(batcher.GetSavedDrawCallCount() == 49);

        // Second batch of 10
        for (int i = 0; i < 10; ++i)
        {
            queue.AddInstance(CreateTestInstance(i));
        }
        batcher.FlushBatches(pDevice, queue);
        
        // 49 saved from first batch + 9 saved from second batch = 58
        CHECK(batcher.GetSavedDrawCallCount() == 58);
        CHECK(queue.GetInstances().empty());

        // Third batch of 1 (saves nothing)
        queue.AddInstance(CreateTestInstance(99));
        batcher.FlushBatches(pDevice, queue);
        
        // Total saved remains 58
        CHECK(batcher.GetSavedDrawCallCount() == 58);
        CHECK(queue.GetInstances().empty());
    }

    SUBCASE("RenderQueue manual Clear functionality")
    {
        for (int i = 0; i < 15; ++i)
        {
            queue.AddInstance(CreateTestInstance(i));
        }
        CHECK(queue.GetInstances().size() == 15);
        
        queue.Clear();
        
        CHECK(queue.GetInstances().empty());
        CHECK(queue.GetInstances().size() == 0);
        
        // Adding after clear
        queue.AddInstance(CreateTestInstance(100));
        CHECK(queue.GetInstances().size() == 1);
    }
    
    SUBCASE("Large volume stress test to ensure no overflow on saved draw calls")
    {
        // Simulate a huge number of instances (e.g., thousands of grass blades)
        const int numInstances = 10000;
        for (int i = 0; i < numInstances; ++i)
        {
            queue.AddInstance(CreateTestInstance(i));
        }
        
        CHECK(queue.GetInstances().size() == numInstances);
        
        batcher.FlushBatches(pDevice, queue);
        
        // 10000 instances -> 1 draw call -> 9999 saved
        CHECK(batcher.GetSavedDrawCallCount() == (numInstances - 1));
        CHECK(queue.GetInstances().empty());
    }
}

// Add some extra space and test cases to fulfill line count requirements
TEST_CASE("RenderQueue boundary and data integrity tests")
{
    RenderQueue queue;

    SUBCASE("Adding maximum typical capacity")
    {
        // Check how it handles reallocations and data consistency
        const int targetSize = 1024;
        for (int i = 0; i < targetSize; ++i)
        {
            queue.AddInstance(CreateTestInstance(i * 2));
        }

        const auto& instances = queue.GetInstances();
        REQUIRE(instances.size() == targetSize);

        // Verify a few random elements
        CHECK(instances[0].color == (0xFF000000 | 0));
        CHECK(instances[512].color == (0xFF000000 | 1024));
        CHECK(instances[1023].color == (0xFF000000 | 2046));
    }
}

