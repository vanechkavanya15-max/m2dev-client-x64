#pragma once

#include <cstdint>
#include <vector>
#include <expected>
#include <span>
#include <atomic>

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#include <d3d9.h>
#else
// Headless test mocks - fully modeling IUnknown and D3D9 inheritance
typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef long HRESULT;
typedef unsigned int UINT;
typedef struct _D3DVERTEXBUFFER_DESC { UINT Size; } D3DVERTEXBUFFER_DESC;
typedef struct _D3DINDEXBUFFER_DESC { UINT Size; } D3DINDEXBUFFER_DESC;
typedef struct _GUID { unsigned long Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; } GUID;
#define S_OK 0
#define E_NOINTERFACE 0x80004002L
#define E_FAIL 0x80004005L
#define E_OUTOFMEMORY 0x8007000EL

struct IUnknown {
    virtual HRESULT QueryInterface(const GUID& riid, void** ppvObject) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    virtual ~IUnknown() = default;
};

struct IDirect3DResource9 : public IUnknown {
    virtual HRESULT GetDevice(void** ppDevice) = 0;
    virtual HRESULT SetPrivateData(const GUID& refguid, const void* pData, DWORD SizeOfData, DWORD Flags) = 0;
    virtual HRESULT GetPrivateData(const GUID& refguid, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT FreePrivateData(const GUID& refguid) = 0;
    virtual DWORD SetPriority(DWORD PriorityNew) = 0;
    virtual DWORD GetPriority() = 0;
    virtual void PreLoad() = 0;
    virtual DWORD GetType() = 0; // D3DRESOURCETYPE
};

struct IDirect3DVertexBuffer9 : public IDirect3DResource9 {
    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) = 0;
    virtual HRESULT Unlock() = 0;
    virtual HRESULT GetDesc(D3DVERTEXBUFFER_DESC* pDesc) = 0;
};

struct IDirect3DIndexBuffer9 : public IDirect3DResource9 {
    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) = 0;
    virtual HRESULT Unlock() = 0;
    virtual HRESULT GetDesc(D3DINDEXBUFFER_DESC* pDesc) = 0;
};
#endif

namespace EterLib::NullRHI
{
    /**
     * @class NullDirect3DVertexBuffer9
     * @brief Atrapa IDirect3DVertexBuffer9 dla testow headless obslugujaca Lock/Unlock w czystym RAM.
     */
    class NullDirect3DVertexBuffer9 final : public IDirect3DVertexBuffer9
    {
    public:
        explicit NullDirect3DVertexBuffer9(UINT length)
        {
            if (length > 0)
            {
                m_data.resize(length);
            }
        }

        ~NullDirect3DVertexBuffer9() override = default;

        // IUnknown
        HRESULT QueryInterface(const GUID& riid, void** ppvObject) override
        {
            if (!ppvObject) return E_NOINTERFACE;
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        
        ULONG AddRef() override
        {
            return ++m_refCount;
        }
        
        ULONG Release() override
        {
            ULONG newCount = --m_refCount;
            if (newCount == 0)
            {
                delete this;
            }
            return newCount;
        }

        // IDirect3DResource9 (Dummy Implementations)
        HRESULT GetDevice(void** ppDevice) override { if(ppDevice) *ppDevice = nullptr; return S_OK; }
        HRESULT SetPrivateData(const GUID& refguid, const void* pData, DWORD SizeOfData, DWORD Flags) override { return S_OK; }
        HRESULT GetPrivateData(const GUID& refguid, void* pData, DWORD* pSizeOfData) override { return S_OK; }
        HRESULT FreePrivateData(const GUID& refguid) override { return S_OK; }
        DWORD SetPriority(DWORD PriorityNew) override { return 0; }
        DWORD GetPriority() override { return 0; }
        void PreLoad() override {}
        DWORD GetType() override { return 1; } // D3DRTYPE_VERTEXBUFFER

        // IDirect3DVertexBuffer9
        HRESULT GetDesc(D3DVERTEXBUFFER_DESC* pDesc) override 
        { 
            if(pDesc) pDesc->Size = static_cast<UINT>(m_data.size()); 
            return S_OK; 
        }

        HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override
        {
            if (!ppbData)
                return E_FAIL;

            if (OffsetToLock >= m_data.size())
                return E_FAIL;

            if (SizeToLock == 0)
            {
                // Jesli SizeToLock == 0 to blokujemy wszystko od OffsetToLock do konca
                *ppbData = m_data.data() + OffsetToLock;
                return S_OK;
            }

            if (static_cast<size_t>(OffsetToLock) + SizeToLock > m_data.size())
                return E_FAIL;

            *ppbData = m_data.data() + OffsetToLock;
            return S_OK;
        }

        HRESULT Unlock() override
        {
            return S_OK;
        }

        // Metody narzedziowe do testow
        std::span<const uint8_t> GetRawData() const noexcept
        {
            return m_data;
        }

    private:
        std::vector<uint8_t> m_data;
        std::atomic<ULONG> m_refCount{1};
    };

    /**
     * @class NullDirect3DIndexBuffer9
     * @brief Atrapa IDirect3DIndexBuffer9 dla testow headless obslugujaca Lock/Unlock w czystym RAM.
     */
    class NullDirect3DIndexBuffer9 final : public IDirect3DIndexBuffer9
    {
    public:
        explicit NullDirect3DIndexBuffer9(UINT length)
        {
            if (length > 0)
            {
                m_data.resize(length);
            }
        }

        ~NullDirect3DIndexBuffer9() override = default;

        // IUnknown
        HRESULT QueryInterface(const GUID& riid, void** ppvObject) override
        {
            if (!ppvObject) return E_NOINTERFACE;
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        
        ULONG AddRef() override
        {
            return ++m_refCount;
        }
        
        ULONG Release() override
        {
            ULONG newCount = --m_refCount;
            if (newCount == 0)
            {
                delete this;
            }
            return newCount;
        }

        // IDirect3DResource9 (Dummy Implementations)
        HRESULT GetDevice(void** ppDevice) override { if(ppDevice) *ppDevice = nullptr; return S_OK; }
        HRESULT SetPrivateData(const GUID& refguid, const void* pData, DWORD SizeOfData, DWORD Flags) override { return S_OK; }
        HRESULT GetPrivateData(const GUID& refguid, void* pData, DWORD* pSizeOfData) override { return S_OK; }
        HRESULT FreePrivateData(const GUID& refguid) override { return S_OK; }
        DWORD SetPriority(DWORD PriorityNew) override { return 0; }
        DWORD GetPriority() override { return 0; }
        void PreLoad() override {}
        DWORD GetType() override { return 2; } // D3DRTYPE_INDEXBUFFER

        // IDirect3DIndexBuffer9
        HRESULT GetDesc(D3DINDEXBUFFER_DESC* pDesc) override 
        { 
            if(pDesc) pDesc->Size = static_cast<UINT>(m_data.size()); 
            return S_OK; 
        }

        HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override
        {
            if (!ppbData)
                return E_FAIL;

            if (OffsetToLock >= m_data.size())
                return E_FAIL;

            if (SizeToLock == 0)
            {
                *ppbData = m_data.data() + OffsetToLock;
                return S_OK;
            }

            if (static_cast<size_t>(OffsetToLock) + SizeToLock > m_data.size())
                return E_FAIL;

            *ppbData = m_data.data() + OffsetToLock;
            return S_OK;
        }

        HRESULT Unlock() override
        {
            return S_OK;
        }

        // Metody narzedziowe do testow
        std::span<const uint8_t> GetRawData() const noexcept
        {
            return m_data;
        }

    private:
        std::vector<uint8_t> m_data;
        std::atomic<ULONG> m_refCount{1};
    };
} // namespace EterLib::NullRHI
