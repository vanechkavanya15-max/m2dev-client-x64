#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#else
#include <cstdint>
#include <cstddef>
// Mock D3D9 types for headless testing
#define D3DUSAGE_DYNAMIC 0x00000200L
#define D3DLOCK_NOOVERWRITE 0x00001000L
#define D3DLOCK_DISCARD 0x00002000L
#define D3DFMT_INDEX16 101
#define D3DFMT_INDEX32 102
#define D3DPOOL_DEFAULT 0

typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef long HRESULT;
#define S_OK 0
#define E_FAIL 0x80004005L
#define FAILED(hr) (((HRESULT)(hr)) < 0)

struct IDirect3DIndexBuffer9 {
    virtual HRESULT Lock(uint32_t OffsetToLock, uint32_t SizeToLock, void** ppbData, DWORD Flags) = 0;
    virtual HRESULT Unlock() = 0;
    virtual ULONG Release() = 0;
};
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9;

struct IDirect3DDevice9 {
    virtual HRESULT CreateIndexBuffer(uint32_t Length, DWORD Usage, DWORD Format, DWORD Pool, IDirect3DIndexBuffer9** ppIndexBuffer, void* pSharedHandle) = 0;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;
typedef DWORD D3DFORMAT;
#endif

namespace EterLib::Render {

class DynamicRingIndexBuffer {
public:
    DynamicRingIndexBuffer();
    ~DynamicRingIndexBuffer();

    // No copy/move semantics for this COM wrapper
    DynamicRingIndexBuffer(const DynamicRingIndexBuffer&) = delete;
    DynamicRingIndexBuffer& operator=(const DynamicRingIndexBuffer&) = delete;
    DynamicRingIndexBuffer(DynamicRingIndexBuffer&&) = delete;
    DynamicRingIndexBuffer& operator=(DynamicRingIndexBuffer&&) = delete;

    bool Initialize(LPDIRECT3DDEVICE9 dev, size_t sizeBytes, D3DFORMAT format);
    uint32_t AllocateIndices(size_t indexCount, const void* indices);
    LPDIRECT3DINDEXBUFFER9 GetBuffer() const noexcept;

private:
    void Destroy();

    LPDIRECT3DDEVICE9 m_device;
    LPDIRECT3DINDEXBUFFER9 m_indexBuffer;
    size_t m_bufferSizeBytes;
    D3DFORMAT m_format;
    size_t m_indexSize; // 2 for INDEX16, 4 for INDEX32
    size_t m_currentOffsetBytes;
};

} // namespace EterLib::Render

