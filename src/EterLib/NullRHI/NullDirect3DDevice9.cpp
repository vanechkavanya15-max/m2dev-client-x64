#include "NullDirect3DDevice9.h"
#include "NullDirect3DBuffer.h"
#include "NullDirect3DTexture.h"

namespace EterLib::NullRHI
{
    NullDirect3DDevice9::NullDirect3DDevice9() noexcept
    {
        m_viewport = D3DVIEWPORT9{0, 0, 1024, 768, 0.0f, 1.0f};
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::QueryInterface(REFIID riid, void** ppvObj)
    {
        if (!ppvObj) return E_FAIL;
        *ppvObj = nullptr;
        // In headless mode we simply provide the device interface
        *ppvObj = static_cast<IDirect3DDevice9*>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE NullDirect3DDevice9::AddRef()
    {
        return ++m_refCount;
    }

    ULONG STDMETHODCALLTYPE NullDirect3DDevice9::Release()
    {
        ULONG count = --m_refCount;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::TestCooperativeLevel() { return D3D_OK; }
    UINT STDMETHODCALLTYPE NullDirect3DDevice9::GetAvailableTextureMem() { return 1024 * 1024 * 512; } // 512 MB mock
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::EvictManagedResources() { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetDirect3D(IDirect3D9** ppD3D9)
    {
        if (ppD3D9) *ppD3D9 = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetDeviceCaps(D3DCAPS9* pCaps)
    {
        if (pCaps) *pCaps = {};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetDisplayMode(UINT, D3DDISPLAYMODE* pMode)
    {
        if (pMode)
        {
            pMode->Width = 1024;
            pMode->Height = 768;
            pMode->RefreshRate = 60;
            pMode->Format = D3DFMT_X8R8G8B8;
        }
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
    {
        if (pParameters) *pParameters = {};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetCursorProperties(UINT, UINT, IDirect3DSurface9*) { return D3D_OK; }
    void STDMETHODCALLTYPE NullDirect3DDevice9::SetCursorPosition(int, int, DWORD) {}
    BOOL STDMETHODCALLTYPE NullDirect3DDevice9::ShowCursor(BOOL) { return 0; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9** ppSwapChain)
    {
        if (ppSwapChain) *ppSwapChain = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetSwapChain(UINT, IDirect3DSwapChain9** ppSwapChain)
    {
        if (ppSwapChain) *ppSwapChain = nullptr;
        return D3D_OK;
    }
    UINT STDMETHODCALLTYPE NullDirect3DDevice9::GetNumberOfSwapChains() { return 1; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::Reset(D3DPRESENT_PARAMETERS*) { return D3D_OK; }
    
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::Present(const D3DRECT*, const D3DRECT*, HWND, const void*)
    {
        m_stats.presentCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetBackBuffer(UINT, UINT, DWORD, IDirect3DSurface9** ppBackBuffer)
    {
        if (ppBackBuffer) *ppBackBuffer = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetRasterStatus(UINT, D3DRASTER_STATUS*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetDialogBoxMode(BOOL) { return D3D_OK; }
    void STDMETHODCALLTYPE NullDirect3DDevice9::SetGammaRamp(UINT, DWORD, const D3DGAMMARAMP*) {}
    void STDMETHODCALLTYPE NullDirect3DDevice9::GetGammaRamp(UINT, D3DGAMMARAMP*) {}

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, void** pSharedHandle)
    {
        if (ppTexture)
        {
            *ppTexture = new (std::nothrow) Client::Graphics::NullRHI::NullDirect3DTexture(Width, Height, Levels, Usage, Format, Pool);
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateVolumeTexture(UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DVolumeTexture9** ppVolumeTexture, void**)
    {
        if (ppVolumeTexture) *ppVolumeTexture = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateCubeTexture(UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DCubeTexture9** ppCubeTexture, void**)
    {
        if (ppCubeTexture) *ppCubeTexture = nullptr;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, void** pSharedHandle)
    {
        if (ppVertexBuffer)
        {
            *ppVertexBuffer = new (std::nothrow) NullDirect3DVertexBuffer9(Length, Usage, FVF, Pool);
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, void** pSharedHandle)
    {
        if (ppIndexBuffer)
        {
            *ppIndexBuffer = new (std::nothrow) NullDirect3DIndexBuffer9(Length, Usage, Format, Pool);
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateRenderTarget(UINT, UINT, D3DFORMAT, DWORD, DWORD, BOOL, IDirect3DSurface9** ppSurface, void**)
    {
        if (ppSurface) *ppSurface = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateDepthStencilSurface(UINT, UINT, D3DFORMAT, DWORD, DWORD, BOOL, IDirect3DSurface9** ppSurface, void**)
    {
        if (ppSurface) *ppSurface = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::UpdateSurface(IDirect3DSurface9*, const D3DRECT*, IDirect3DSurface9*, const void*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::UpdateTexture(IDirect3DBaseTexture9*, IDirect3DBaseTexture9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetFrontBufferData(UINT, IDirect3DSurface9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::StretchRect(IDirect3DSurface9*, const D3DRECT*, IDirect3DSurface9*, const D3DRECT*, DWORD) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::ColorFill(IDirect3DSurface9*, const D3DRECT*, D3DCOLOR) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateOffscreenPlainSurface(UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9** ppSurface, void**)
    {
        if (ppSurface) *ppSurface = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetRenderTarget(DWORD, IDirect3DSurface9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetRenderTarget(DWORD, IDirect3DSurface9** ppRenderTarget)
    {
        if (ppRenderTarget) *ppRenderTarget = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetDepthStencilSurface(IDirect3DSurface9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
    {
        if (ppZStencilSurface) *ppZStencilSurface = nullptr;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::BeginScene()
    {
        m_stats.beginSceneCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::EndScene()
    {
        m_stats.endSceneCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::Clear(DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD)
    {
        m_stats.clearCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetTransform(D3DTRANSFORMSTATETYPE, D3DMATRIX* pMatrix)
    {
        if (pMatrix) *pMatrix = {};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) { return D3D_OK; }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetViewport(const D3DVIEWPORT9* pViewport)
    {
        if (pViewport) m_viewport = *pViewport;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetViewport(D3DVIEWPORT9* pViewport)
    {
        if (pViewport) *pViewport = m_viewport;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetMaterial(const D3DMATERIAL9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetMaterial(D3DMATERIAL9* pMaterial)
    {
        if (pMaterial) *pMaterial = {};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetLight(DWORD, const D3DLIGHT9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetLight(DWORD, D3DLIGHT9* pLight)
    {
        if (pLight) *pLight = {};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::LightEnable(DWORD, BOOL) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetLightEnable(DWORD, BOOL* pEnable)
    {
        if (pEnable) *pEnable = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetClipPlane(DWORD, const float*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetClipPlane(DWORD, float*) { return D3D_OK; }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetRenderState(D3DRENDERSTATETYPE, DWORD)
    {
        m_stats.setRenderStateCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetRenderState(D3DRENDERSTATETYPE, DWORD* pValue)
    {
        if (pValue) *pValue = 0;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateStateBlock(DWORD, IDirect3DStateBlock9** ppSB)
    {
        if (ppSB) *ppSB = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::BeginStateBlock() { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::EndStateBlock(IDirect3DStateBlock9** ppSB)
    {
        if (ppSB) *ppSB = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetClipStatus(const void*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetClipStatus(void*) { return D3D_OK; }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetTexture(DWORD, IDirect3DBaseTexture9** ppTexture)
    {
        if (ppTexture) *ppTexture = nullptr;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetTexture(DWORD, IDirect3DBaseTexture9*)
    {
        m_stats.setTextureCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD* pValue)
    {
        if (pValue) *pValue = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD* pValue)
    {
        if (pValue) *pValue = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::ValidateDevice(DWORD* pNumPasses)
    {
        if (pNumPasses) *pNumPasses = 1;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetPaletteEntries(UINT, const void*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetPaletteEntries(UINT, void*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetCurrentTexturePalette(UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetCurrentTexturePalette(UINT *PaletteNumber)
    {
        if (PaletteNumber) *PaletteNumber = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetScissorRect(CONST RECT*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetScissorRect(RECT*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetSoftwareVertexProcessing(BOOL) { return D3D_OK; }
    BOOL STDMETHODCALLTYPE NullDirect3DDevice9::GetSoftwareVertexProcessing() { return 0; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetNPatchMode(float) { return D3D_OK; }
    float STDMETHODCALLTYPE NullDirect3DDevice9::GetNPatchMode() { return 0.0f; }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE, UINT, UINT)
    {
        m_stats.drawCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE, int, UINT, UINT, UINT, UINT)
    {
        m_stats.drawIndexedCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE, UINT, const void*, UINT)
    {
        m_stats.drawUPCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*, UINT)
    {
        m_stats.drawIndexedCalls++;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::ProcessVertices(UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*, DWORD) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9*, IDirect3DVertexDeclaration9** ppDecl)
    {
        if (ppDecl) *ppDecl = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
    {
        if (ppDecl) *ppDecl = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetFVF(DWORD) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetFVF(DWORD* pFVF)
    {
        if (pFVF) *pFVF = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateVertexShader(const DWORD*, IDirect3DVertexShader9** ppShader)
    {
        if (ppShader) *ppShader = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetVertexShader(IDirect3DVertexShader9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetVertexShader(IDirect3DVertexShader9** ppShader)
    {
        if (ppShader) *ppShader = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetVertexShaderConstantF(UINT, const float*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetVertexShaderConstantF(UINT, float*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetVertexShaderConstantI(UINT, const int*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetVertexShaderConstantI(UINT, int*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetVertexShaderConstantB(UINT, const BOOL*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetVertexShaderConstantB(UINT, BOOL*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetStreamSource(UINT, IDirect3DVertexBuffer9*, UINT, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetStreamSource(UINT, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
    {
        if (ppStreamData) *ppStreamData = nullptr;
        if (pOffsetInBytes) *pOffsetInBytes = 0;
        if (pStride) *pStride = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetStreamSourceFreq(UINT, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetStreamSourceFreq(UINT, UINT* pSetting)
    {
        if (pSetting) *pSetting = 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetIndices(IDirect3DIndexBuffer9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
    {
        if (ppIndexData) *ppIndexData = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreatePixelShader(const DWORD*, IDirect3DPixelShader9** ppShader)
    {
        if (ppShader) *ppShader = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetPixelShader(IDirect3DPixelShader9*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetPixelShader(IDirect3DPixelShader9** ppShader)
    {
        if (ppShader) *ppShader = nullptr;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetPixelShaderConstantF(UINT, const float*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetPixelShaderConstantF(UINT, float*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetPixelShaderConstantI(UINT, const int*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::GetPixelShaderConstantI(UINT, int*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::SetPixelShaderConstantB(UINT, const BOOL*, UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawRectPatch(UINT, const float*, CONST D3DRECTPATCH_INFO*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DrawTriPatch(UINT, const float*, CONST D3DTRIPATCH_INFO*) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::DeletePatch(UINT) { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE NullDirect3DDevice9::CreateQuery(D3DQUERYTYPE, IDirect3DQuery9** ppQuery)
    {
        if (ppQuery) *ppQuery = nullptr;
        return D3D_OK;
    }
}
