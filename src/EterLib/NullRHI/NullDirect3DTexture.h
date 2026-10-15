#pragma once

#include <d3d9.h>
#include <vector>
#include <atomic>
#include <cstdint>
#include <span>
#include <memory>

namespace Client::Graphics::NullRHI
{

class NullDirect3DSurface final : public IDirect3DSurface9
{
public:
    NullDirect3DSurface(UINT width, UINT height, D3DFORMAT format)
        : m_width(width), m_height(height), m_format(format)
    {
        m_pitch = m_width * 4; 
        m_pixels.resize(static_cast<size_t>(m_pitch) * m_height, 0);
    }
    
    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    ULONG STDMETHODCALLTYPE Release() override 
    { 
        ULONG count = --m_refCount;
        if (count == 0) delete this;
        return count;
    }
    
    // IDirect3DResource9
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return E_NOTIMPL; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_SURFACE; }

    // IDirect3DSurface9
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) override 
    { 
        if (!pDesc) return D3DERR_INVALIDCALL;
        pDesc->Format = m_format;
        pDesc->Type = D3DRTYPE_SURFACE;
        pDesc->Usage = 0;
        pDesc->Pool = D3DPOOL_MANAGED;
        pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
        pDesc->MultiSampleQuality = 0;
        pDesc->Width = m_width;
        pDesc->Height = m_height;
        return D3D_OK; 
    }
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) override 
    { 
        if (!pLockedRect) return D3DERR_INVALIDCALL;
        if (m_isLocked) return D3DERR_INVALIDCALL;
        
        m_isLocked = true;
        pLockedRect->Pitch = m_pitch;
        
        if (pRect)
        {
            if (pRect->left < 0 || pRect->right > static_cast<LONG>(m_width) || 
                pRect->top < 0 || pRect->bottom > static_cast<LONG>(m_height) ||
                pRect->left >= pRect->right || pRect->top >= pRect->bottom)
            {
                m_isLocked = false;
                return D3DERR_INVALIDCALL;
            }
            
            pLockedRect->pBits = m_pixels.data() + static_cast<size_t>(pRect->top) * m_pitch + static_cast<size_t>(pRect->left) * 4;
        }
        else
        {
            pLockedRect->pBits = m_pixels.data();
        }
        return D3D_OK; 
    }
    HRESULT STDMETHODCALLTYPE UnlockRect() override 
    { 
        if (!m_isLocked) return D3DERR_INVALIDCALL;
        m_isLocked = false;
        return D3D_OK; 
    }
    HRESULT STDMETHODCALLTYPE GetDC(HDC* phdc) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hdc) override { return E_NOTIMPL; }

private:
    std::atomic<ULONG> m_refCount{1};
    UINT m_width;
    UINT m_height;
    D3DFORMAT m_format;
    UINT m_pitch;
    std::vector<uint8_t> m_pixels;
    bool m_isLocked{false};
};

struct ComReleaseDeleter
{
    template <typename T>
    void operator()(T* p) const noexcept
    {
        if (p) p->Release();
    }
};

template<typename T>
using ComPtr = std::unique_ptr<T, ComReleaseDeleter>;

class NullDirect3DTexture final : public IDirect3DTexture9
{
public:
    NullDirect3DTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
        : m_width(width), m_height(height), m_usage(usage), m_format(format), m_pool(pool)
    {
        if (levels == 0)
        {
            levels = 1;
            UINT w = width, h = height;
            while (w > 1 || h > 1)
            {
                if (w > 1) w >>= 1;
                if (h > 1) h >>= 1;
                levels++;
            }
        }
        m_levels = levels;
        
        m_surfaces.reserve(levels);
        UINT w = width, h = height;
        for (UINT i = 0; i < levels; ++i)
        {
            ComPtr<NullDirect3DSurface> surface(new NullDirect3DSurface(w, h, format));
            m_surfaces.push_back(std::move(surface));
            
            if (w > 1) w >>= 1;
            if (h > 1) h >>= 1;
        }
    }
    
    ~NullDirect3DTexture() = default; 
    
    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    ULONG STDMETHODCALLTYPE Release() override 
    { 
        ULONG count = --m_refCount;
        if (count == 0) delete this;
        return count;
    }
    
    // IDirect3DBaseTexture9
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return E_NOTIMPL; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_TEXTURE; }
    DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override { return 0; }
    DWORD STDMETHODCALLTYPE GetLOD() override { return 0; }
    DWORD STDMETHODCALLTYPE GetLevelCount() override { return m_levels; }
    HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override { return E_NOTIMPL; }
    D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return D3DTEXF_NONE; }
    void STDMETHODCALLTYPE GenerateMipSubLevels() override {}

    // IDirect3DTexture9
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override 
    {
        if (Level >= m_levels) return D3DERR_INVALIDCALL;
        return m_surfaces[Level]->GetDesc(pDesc);
    }
    HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface9** ppSurfaceLevel) override 
    {
        if (Level >= m_levels || !ppSurfaceLevel) return D3DERR_INVALIDCALL;
        *ppSurfaceLevel = m_surfaces[Level].get();
        (*ppSurfaceLevel)->AddRef();
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) override 
    {
        if (Level >= m_levels) return D3DERR_INVALIDCALL;
        return m_surfaces[Level]->LockRect(pLockedRect, pRect, Flags);
    }
    HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level) override 
    {
        if (Level >= m_levels) return D3DERR_INVALIDCALL;
        return m_surfaces[Level]->UnlockRect();
    }
    HRESULT STDMETHODCALLTYPE AddDirtyRect(CONST RECT* pDirtyRect) override { return D3D_OK; }

private:
    std::atomic<ULONG> m_refCount{1};
    UINT m_width;
    UINT m_height;
    DWORD m_usage;
    D3DFORMAT m_format;
    D3DPOOL m_pool;
    UINT m_levels;
    std::vector<ComPtr<NullDirect3DSurface>> m_surfaces;
};

} // namespace Client::Graphics::NullRHI
