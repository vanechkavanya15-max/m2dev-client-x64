#define TEST_MODE_DISABLE_STDAFX 1
#include "doctest.h"
#include "../src/EterLib/Render/DynamicRingIndexBuffer.h"

#include <vector>
#include <cstring>
#include <iostream>

using namespace EterLib::Render;

// Mocks for D3D9 interfaces
struct MockIndexBuffer : public IDirect3DIndexBuffer9 {
    std::vector<uint8_t> data;
    DWORD lastLockFlags = 0;
    uint32_t lastOffset = 0;
    uint32_t lastSize = 0;
    bool isLocked = false;
    ULONG refCount = 1;

    MockIndexBuffer(size_t size) : data(size) {}

    HRESULT Lock(uint32_t OffsetToLock, uint32_t SizeToLock, void** ppbData, DWORD Flags) override {
        if (isLocked) return E_FAIL;
        if (OffsetToLock + SizeToLock > data.size()) return E_FAIL;
        
        lastOffset = OffsetToLock;
        lastSize = SizeToLock;
        lastLockFlags = Flags;
        isLocked = true;
        
        *ppbData = data.data() + OffsetToLock;
        return S_OK;
    }

    HRESULT Unlock() override {
        if (!isLocked) return E_FAIL;
        isLocked = false;
        return S_OK;
    }

    ULONG Release() override {
        refCount--;
        if (refCount == 0) {
            delete this;
            return 0;
        }
        return refCount;
    }
};

struct MockDevice : public IDirect3DDevice9 {
    HRESULT CreateIndexBuffer(uint32_t Length, DWORD Usage, DWORD Format, DWORD Pool, IDirect3DIndexBuffer9** ppIndexBuffer, void* pSharedHandle) override {
        *ppIndexBuffer = new MockIndexBuffer(Length);
        return S_OK;
    }
};

TEST_SUITE("DynamicRingIndexBuffer") {
    TEST_CASE("Initialization and cleanup") {
        MockDevice device;
        DynamicRingIndexBuffer ibo;
        
        CHECK(ibo.Initialize(&device, 1024, D3DFMT_INDEX16) == true);
        CHECK(ibo.GetBuffer() != nullptr);
        
        // Invalid format
        CHECK(ibo.Initialize(&device, 1024, 999) == false);
        CHECK(ibo.GetBuffer() == nullptr);
    }

    TEST_CASE("Allocate INDEX16") {
        MockDevice device;
        DynamicRingIndexBuffer ibo;
        
        REQUIRE(ibo.Initialize(&device, 16, D3DFMT_INDEX16)); // 16 bytes = 8 indices
        
        uint16_t data1[] = {1, 2, 3};
        uint32_t startIdx1 = ibo.AllocateIndices(3, data1);
        CHECK(startIdx1 == 0);
        
        uint16_t data2[] = {4, 5};
        uint32_t startIdx2 = ibo.AllocateIndices(2, data2);
        CHECK(startIdx2 == 3);
        
        // Exceeds total capacity
        uint16_t data3[9] = {0};
        CHECK(ibo.AllocateIndices(9, data3) == 0xFFFFFFFF);
    }

    TEST_CASE("Allocate INDEX32") {
        MockDevice device;
        DynamicRingIndexBuffer ibo;
        
        REQUIRE(ibo.Initialize(&device, 32, D3DFMT_INDEX32)); // 32 bytes = 8 indices
        
        uint32_t data1[] = {10, 20, 30};
        uint32_t startIdx1 = ibo.AllocateIndices(3, data1);
        CHECK(startIdx1 == 0);
        
        uint32_t data2[] = {40, 50};
        uint32_t startIdx2 = ibo.AllocateIndices(2, data2);
        CHECK(startIdx2 == 3);
        
        // Verify buffer content
        MockIndexBuffer* buffer = static_cast<MockIndexBuffer*>(ibo.GetBuffer());
        uint32_t* ptr = reinterpret_cast<uint32_t*>(buffer->data.data());
        CHECK(ptr[0] == 10);
        CHECK(ptr[1] == 20);
        CHECK(ptr[2] == 30);
        CHECK(ptr[3] == 40);
        CHECK(ptr[4] == 50);
    }

    TEST_CASE("Ring buffer DISCARD behavior") {
        MockDevice device;
        DynamicRingIndexBuffer ibo;
        
        // 4MB capacity, INDEX16
        const size_t capacity = 4 * 1024 * 1024;
        REQUIRE(ibo.Initialize(&device, capacity, D3DFMT_INDEX16));
        
        std::vector<uint16_t> bigData(1000, 42);
        
        // First allocation should use NOOVERWRITE
        uint32_t start1 = ibo.AllocateIndices(bigData.size(), bigData.data());
        CHECK(start1 == 0);
        MockIndexBuffer* buffer = static_cast<MockIndexBuffer*>(ibo.GetBuffer());
        CHECK(buffer->lastLockFlags == D3DLOCK_NOOVERWRITE);
        
        // Manually move offset to near end
        size_t offsetRemaining = capacity - (bigData.size() * 2);
        std::vector<uint16_t> padding(offsetRemaining / 2 - 10, 0); // leave space for 10 indices
        ibo.AllocateIndices(padding.size(), padding.data());
        
        // Now request 20 indices, which should cause a wrap-around
        std::vector<uint16_t> wrapData(20, 99);
        uint32_t startWrap = ibo.AllocateIndices(wrapData.size(), wrapData.data());
        
        CHECK(startWrap == 0);
        CHECK(buffer->lastLockFlags == D3DLOCK_DISCARD);
    }
}

