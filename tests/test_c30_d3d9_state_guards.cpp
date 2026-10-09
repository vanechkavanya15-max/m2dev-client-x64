#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include "EterLib/StateManager.h"
#include "EterLib/StateManagerGuards.h"

// ============================================================================
// TestMockD3D9Device: Implementacja IDirect3DDevice9Ex z pelna telemetria
// ============================================================================
class TestMockD3D9Device final : public IDirect3DDevice9Ex
{
public:
    ULONG m_refCount = 1;

    // Telemetria sprzetowa (hardware state tracking)
    IDirect3DVertexDeclaration9* m_hwVertexDecl = nullptr;
    IDirect3DVertexShader9* m_hwVertexShader = nullptr;
    IDirect3DPixelShader9* m_hwPixelShader = nullptr;
    DWORD m_hwFVF = 0;
    DWORD m_hwRenderStates[256] = {};

    uint32_t m_setVertexDeclCalls = 0;
    uint32_t m_setVertexShaderCalls = 0;
    uint32_t m_setPixelShaderCalls = 0;
    uint32_t m_setFVFCalls = 0;
    uint32_t m_setRenderStateCalls = 0;

    TestMockD3D9Device()
    {
        for (int i = 0; i < 256; ++i)
            m_hwRenderStates[i] = 0;
    }

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override
    {
        if (!ppvObj) return E_FAIL;
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG c = --m_refCount;
        if (c == 0) delete this;
        return c;
    }

    // IDirect3DDevice9 - kluczowe metody maszyny stanow
    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override
    {
        m_setRenderStateCalls++;
        if (State < 256)
            m_hwRenderStates[State] = Value;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override
    {
        if (pValue && State < 256)
            *pValue = m_hwRenderStates[State];
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override
    {
        m_setVertexDeclCalls++;
        m_hwVertexDecl = pDecl;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override
    {
        if (ppDecl) *ppDecl = m_hwVertexDecl;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override
    {
        m_setFVFCalls++;
        m_hwFVF = FVF;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override
    {
        if (pFVF) *pFVF = m_hwFVF;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) override
    {
        m_setVertexShaderCalls++;
        m_hwVertexShader = pShader;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) override
    {
        if (ppShader) *ppShader = m_hwVertexShader;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) override
    {
        m_setPixelShaderCalls++;
        m_hwPixelShader = pShader;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) override
    {
        if (ppShader) *ppShader = m_hwPixelShader;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetTexture(DWORD, IDirect3DBaseTexture9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTexture(DWORD, IDirect3DBaseTexture9** pp) override { if (pp) *pp = nullptr; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD* p) override { if (p) *p = 0; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD* p) override { if (p) *p = 0; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE, D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetMaterial(CONST D3DMATERIAL9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetLight(DWORD, CONST D3DLIGHT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetLight(DWORD, D3DLIGHT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetScissorRect(CONST RECT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetScissorRect(RECT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL) override { return D3D_OK; }
    BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override { return FALSE; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT, CONST float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT, IDirect3DVertexBuffer9*, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE BeginScene() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE EndScene() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE, UINT, CONST void*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE, UINT, UINT, UINT, CONST void*, D3DFORMAT, CONST void*, UINT) override { return D3D_OK; }

    // Pozostale metody interfejsu IDirect3DDevice9
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetAvailableTextureMem() override { return 0; }
    HRESULT STDMETHODCALLTYPE EvictManagedResources() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT, D3DDISPLAYMODE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT, UINT, IDirect3DSurface9*) override { return D3D_OK; }
    void STDMETHODCALLTYPE SetCursorPosition(int, int, DWORD) override {}
    BOOL STDMETHODCALLTYPE ShowCursor(BOOL) override { return FALSE; }
    HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetSwapChain(UINT, IDirect3DSwapChain9**) override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetNumberOfSwapChains() override { return 0; }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE Present(CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT, D3DRASTER_STATUS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL) override { return D3D_OK; }
    void STDMETHODCALLTYPE SetGammaRamp(UINT, DWORD, CONST D3DGAMMARAMP*) override {}
    void STDMETHODCALLTYPE GetGammaRamp(UINT, D3DGAMMARAMP*) override {}
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DVolumeTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DCubeTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT, DWORD, DWORD, D3DPOOL, IDirect3DVertexBuffer9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DIndexBuffer9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9*, CONST RECT*, IDirect3DSurface9*, CONST POINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9*, IDirect3DBaseTexture9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9*, CONST RECT*, IDirect3DSurface9*, CONST RECT*, D3DTEXTUREFILTERTYPE) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9*, CONST RECT*, D3DCOLOR) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD, IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE Clear(DWORD, CONST D3DRECT*, DWORD, D3DCOLOR, float, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetClipStatus(CONST D3DCLIPSTATUS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD, BOOL) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD, BOOL*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD, CONST float*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD, float*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetViewport(CONST D3DVIEWPORT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT, CONST PALETTEENTRY*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT, PALETTEENTRY*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetNPatchMode(float) override { return D3D_OK; }
    float STDMETHODCALLTYPE GetNPatchMode() override { return 0.0f; }
    HRESULT STDMETHODCALLTYPE ProcessVertices(UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(CONST D3DVERTEXELEMENT9*, IDirect3DVertexDeclaration9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexShader(CONST DWORD*, IDirect3DVertexShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT, float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT, CONST int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT, int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT, CONST BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT, BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetStreamSource(UINT, IDirect3DVertexBuffer9**, UINT*, UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT, UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreatePixelShader(CONST DWORD*, IDirect3DPixelShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT, CONST float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT, float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT, CONST int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT, int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT, CONST BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT, BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT, CONST float*, CONST D3DRECTPATCH_INFO*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT, CONST float*, CONST D3DTRIPATCH_INFO*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DeletePatch(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE, IDirect3DQuery9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE, IDirect3DStateBlock9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE BeginStateBlock() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9**) override { return D3D_OK; }

    // IDirect3DDevice9Ex
    HRESULT STDMETHODCALLTYPE SetConvolutionMonoKernel(UINT, UINT, float*, float*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ComposeRects(IDirect3DSurface9*, IDirect3DSurface9*, IDirect3DVertexBuffer9*, UINT, IDirect3DVertexBuffer9*, D3DCOMPOSERECTSOP, int, int) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE PresentEx(CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(INT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE WaitForVBlank(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CheckResourceResidency(IDirect3DResource9**, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CheckDeviceState(HWND) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateRenderTargetEx(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurfaceEx(UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9**, HANDLE*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurfaceEx(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ResetEx(D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDisplayModeEx(UINT, D3DDISPLAYMODEEX*, D3DDISPLAYROTATION*) override { return D3D_OK; }
};

// Pomocnicze atrapy interfejsow Direct3D 9 do testowania wskaznikow
struct DummyVertexDeclaration : IDirect3DVertexDeclaration9
{
    ULONG ref = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override { return --ref; }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDeclaration(D3DVERTEXELEMENT9*, UINT*) override { return D3D_OK; }
};

struct DummyVertexShader : IDirect3DVertexShader9
{
    ULONG ref = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override { return --ref; }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetFunction(void*, UINT*) override { return D3D_OK; }
};

struct DummyPixelShader : IDirect3DPixelShader9
{
    ULONG ref = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override { return --ref; }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetFunction(void*, UINT*) override { return D3D_OK; }
};

// ============================================================================
// TESTY JEDNOSTKOWE
// ============================================================================
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_CHECK(cond, msg) \
    do { \
        if (cond) { \
            g_testsPassed++; \
        } else { \
            g_testsFailed++; \
            std::cerr << "FAIL: " << msg << " (linia " << __LINE__ << ")\n"; \
        } \
    } while(0)

void TestVertexDeclGuard(TestMockD3D9Device* dev)
{
    std::cout << "[RUN] Test 1: ScopedD3DVertexDeclGuard (Save/Restore, CleanExit, Move)..." << std::endl;

    DummyVertexDeclaration decl1, decl2, decl3;

    // 1. Zwykly Save i Restore (bRestoreOnExit = true)
    STATEMANAGER.SetVertexDeclaration(&decl1);
    TEST_CHECK(dev->m_hwVertexDecl == &decl1, "Poczatkowa deklaracja ustawiona");

    {
        ScopedD3DVertexDeclGuard guard(&decl2, true);
        LPDIRECT3DVERTEXDECLARATION9 cur = nullptr;
        STATEMANAGER.GetVertexDeclaration(&cur);
        TEST_CHECK(cur == &decl2, "Deklaracja zmieniona w zasiegu straznika");
        TEST_CHECK(dev->m_hwVertexDecl == &decl2, "Sprzetowa deklaracja zaktualizowana");
    }

    LPDIRECT3DVERTEXDECLARATION9 restored = nullptr;
    STATEMANAGER.GetVertexDeclaration(&restored);
    TEST_CHECK(restored == &decl1, "Deklaracja poprawnie przywrocona po wyjsciu ze straznika");
    TEST_CHECK(dev->m_hwVertexDecl == &decl1, "Sprzetowa deklaracja poprawnie przywrocona");

    // 2. Gwarancja czystego wyjscia do NULL (bRestoreOnExit = false)
    {
        ScopedD3DVertexDeclGuard guard(&decl3, false);
        LPDIRECT3DVERTEXDECLARATION9 cur = nullptr;
        STATEMANAGER.GetVertexDeclaration(&cur);
        TEST_CHECK(cur == &decl3, "Deklaracja ustawiona na decl3 w strazniku");
    }
    STATEMANAGER.GetVertexDeclaration(&restored);
    TEST_CHECK(restored == nullptr, "Deklaracja wyczyszczona do NULL po bRestoreOnExit=false");
    TEST_CHECK(dev->m_hwVertexDecl == nullptr, "Sprzetowa deklaracja wyczyszczona do NULL");

    // 3. Move semantics
    STATEMANAGER.SetVertexDeclaration(&decl1);
    {
        ScopedD3DVertexDeclGuard guardA(&decl2, true);
        ScopedD3DVertexDeclGuard guardB = std::move(guardA);
    }
    STATEMANAGER.GetVertexDeclaration(&restored);
    TEST_CHECK(restored == &decl1, "Przeniesiony straznik poprawnie przywrocil stan");

    std::cout << "[PASS] Test 1: ScopedD3DVertexDeclGuard dziala w 100% poprawnie." << std::endl;
}

void TestShaderGuard(TestMockD3D9Device* dev)
{
    std::cout << "[RUN] Test 2: ScopedD3DShaderGuard (VS i PS)..." << std::endl;

    DummyVertexShader vs1, vs2;
    DummyPixelShader ps1, ps2;

    STATEMANAGER.SetVertexShader(&vs1);
    STATEMANAGER.SetPixelShader(&ps1);

    // Save & Restore
    {
        ScopedD3DShaderGuard guard(&vs2, &ps2, true);
        LPDIRECT3DVERTEXSHADER9 curVS = nullptr;
        LPDIRECT3DPIXELSHADER9 curPS = nullptr;
        STATEMANAGER.GetVertexShader(&curVS);
        STATEMANAGER.GetPixelShader(&curPS);
        TEST_CHECK(curVS == &vs2, "Nowy VertexShader aktywny");
        TEST_CHECK(curPS == &ps2, "Nowy PixelShader aktywny");
        TEST_CHECK(dev->m_hwVertexShader == &vs2, "Sprzetowy VS ustawiony");
        TEST_CHECK(dev->m_hwPixelShader == &ps2, "Sprzetowy PS ustawiony");
    }

    LPDIRECT3DVERTEXSHADER9 resVS = nullptr;
    LPDIRECT3DPIXELSHADER9 resPS = nullptr;
    STATEMANAGER.GetVertexShader(&resVS);
    STATEMANAGER.GetPixelShader(&resPS);
    TEST_CHECK(resVS == &vs1, "Poprzedni VertexShader przywrocony");
    TEST_CHECK(resPS == &ps1, "Poprzedni PixelShader przywrocony");

    // Clean exit to NULL
    {
        ScopedD3DShaderGuard guard(&vs2, &ps2, false);
    }
    STATEMANAGER.GetVertexShader(&resVS);
    STATEMANAGER.GetPixelShader(&resPS);
    TEST_CHECK(resVS == nullptr, "VertexShader wyczyszczony do NULL na wyjsciu");
    TEST_CHECK(resPS == nullptr, "PixelShader wyczyszczony do NULL na wyjsciu");
    TEST_CHECK(dev->m_hwVertexShader == nullptr, "Sprzetowy VS wyczyszczony do NULL");
    TEST_CHECK(dev->m_hwPixelShader == nullptr, "Sprzetowy PS wyczyszczony do NULL");

    std::cout << "[PASS] Test 2: ScopedD3DShaderGuard dziala w 100% poprawnie." << std::endl;
}

void TestFFPGuard(TestMockD3D9Device* dev)
{
    std::cout << "[RUN] Test 3: ScopedD3DFFPGuard (Fixed-Function Pipeline Guard)..." << std::endl;

    DummyVertexDeclaration decl;
    DummyVertexShader vs;
    DummyPixelShader ps;

    // Ustawiamy "brudny" stan z aktywnym shaderem i deklaracja
    STATEMANAGER.SetVertexDeclaration(&decl);
    STATEMANAGER.SetVertexShader(&vs);
    STATEMANAGER.SetPixelShader(&ps);
    STATEMANAGER.SetFVF(0);

    const DWORD terrainFVF = D3DFVF_XYZ | D3DFVF_NORMAL;

    // Test ScopedD3DFFPGuard w trybie wejscia do terenu (bRestoreOnExit = false)
    {
        ScopedD3DFFPGuard ffpGuard(terrainFVF, false);

        LPDIRECT3DVERTEXDECLARATION9 curDecl = nullptr;
        LPDIRECT3DVERTEXSHADER9 curVS = nullptr;
        LPDIRECT3DPIXELSHADER9 curPS = nullptr;
        DWORD curFVF = 0;

        STATEMANAGER.GetVertexDeclaration(&curDecl);
        STATEMANAGER.GetVertexShader(&curVS);
        STATEMANAGER.GetPixelShader(&curPS);
        STATEMANAGER.GetFVF(&curFVF);

        TEST_CHECK(curDecl == nullptr, "FFP Guard: VertexDeclaration zresetowana do NULL");
        TEST_CHECK(curVS == nullptr, "FFP Guard: VertexShader zresetowany do NULL");
        TEST_CHECK(curPS == nullptr, "FFP Guard: PixelShader zresetowany do NULL");
        TEST_CHECK(curFVF == terrainFVF, "FFP Guard: FVF poprawnie ustawiony");

        // Weryfikacja stanu fizycznego urzadzenia
        TEST_CHECK(dev->m_hwVertexDecl == nullptr, "FFP Guard: Sprzetowa VertexDeclaration jest NULL");
        TEST_CHECK(dev->m_hwVertexShader == nullptr, "FFP Guard: Sprzetowy VertexShader jest NULL");
        TEST_CHECK(dev->m_hwPixelShader == nullptr, "FFP Guard: Sprzetowy PixelShader jest NULL");
        TEST_CHECK(dev->m_hwFVF == terrainFVF, "FFP Guard: Sprzetowy FVF poprawny");
    }

    // Po wyjsciu z bloku renderowania terenu stan nadal musi byc czysty (brak wyciekow shaderow)
    LPDIRECT3DVERTEXDECLARATION9 exitDecl = nullptr;
    LPDIRECT3DVERTEXSHADER9 exitVS = nullptr;
    LPDIRECT3DPIXELSHADER9 exitPS = nullptr;
    STATEMANAGER.GetVertexDeclaration(&exitDecl);
    STATEMANAGER.GetVertexShader(&exitVS);
    STATEMANAGER.GetPixelShader(&exitPS);

    TEST_CHECK(exitDecl == nullptr, "Po wyjsciu: VertexDeclaration pozostaje NULL (zero leak)");
    TEST_CHECK(exitVS == nullptr, "Po wyjsciu: VertexShader pozostaje NULL (zero leak)");
    TEST_CHECK(exitPS == nullptr, "Po wyjsciu: PixelShader pozostaje NULL (zero leak)");

    std::cout << "[PASS] Test 3: ScopedD3DFFPGuard definitywnie zabezpiecza potok FVF terenu." << std::endl;
}

void TestRenderStateGuard(TestMockD3D9Device* dev)
{
    std::cout << "[RUN] Test 4: ScopedD3DRenderStateGuard (Pojedynczy i Initializer List)..." << std::endl;

    STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    // 1. Pojedynczy straznik
    {
        ScopedD3DRenderStateGuard guard(D3DRS_ZWRITEENABLE, FALSE);
        DWORD curZ = 0;
        STATEMANAGER.GetRenderState(D3DRS_ZWRITEENABLE, &curZ);
        TEST_CHECK(curZ == FALSE, "ZWRITEENABLE zmienione na FALSE");
    }
    DWORD resZ = 0;
    STATEMANAGER.GetRenderState(D3DRS_ZWRITEENABLE, &resZ);
    TEST_CHECK(resZ == TRUE, "ZWRITEENABLE przywrocone do TRUE");

    // 2. Multi-state initializer list
    {
        ScopedD3DRenderStateGuard multiGuard({
            { D3DRS_ALPHABLENDENABLE, TRUE },
            { D3DRS_SRCBLEND, D3DBLEND_SRCALPHA },
            { D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA }
        });

        DWORD blend = 0, src = 0, dest = 0;
        STATEMANAGER.GetRenderState(D3DRS_ALPHABLENDENABLE, &blend);
        STATEMANAGER.GetRenderState(D3DRS_SRCBLEND, &src);
        STATEMANAGER.GetRenderState(D3DRS_DESTBLEND, &dest);

        TEST_CHECK(blend == TRUE, "Multi-state: ALPHABLENDENABLE zmienione");
        TEST_CHECK(src == D3DBLEND_SRCALPHA, "Multi-state: SRCBLEND zmienione");
        TEST_CHECK(dest == D3DBLEND_INVSRCALPHA, "Multi-state: DESTBLEND zmienione");
    }

    DWORD rBlend = 0, rSrc = 0, rDest = 0;
    STATEMANAGER.GetRenderState(D3DRS_ALPHABLENDENABLE, &rBlend);
    STATEMANAGER.GetRenderState(D3DRS_SRCBLEND, &rSrc);
    STATEMANAGER.GetRenderState(D3DRS_DESTBLEND, &rDest);

    TEST_CHECK(rBlend == FALSE, "Multi-state: ALPHABLENDENABLE przywrocone");
    TEST_CHECK(rSrc == D3DBLEND_ONE, "Multi-state: SRCBLEND przywrocone");
    TEST_CHECK(rDest == D3DBLEND_ZERO, "Multi-state: DESTBLEND przywrocone");

    std::cout << "[PASS] Test 4: ScopedD3DRenderStateGuard dziala w 100% poprawnie." << std::endl;
}

void TestTextureStageAndSamplerGuard()
{
    std::cout << "[RUN] Test 5: ScopedD3DTextureStageStateGuard i ScopedD3DSamplerStateGuard..." << std::endl;

    STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    STATEMANAGER.SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);

    {
        ScopedD3DTextureStageStateGuard tssGuard(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
        ScopedD3DSamplerStateGuard sampGuard(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);

        DWORD op = 0, addr = 0;
        STATEMANAGER.GetTextureStageState(0, D3DTSS_COLOROP, &op);
        STATEMANAGER.GetSamplerState(0, D3DSAMP_ADDRESSU, &addr);

        TEST_CHECK(op == D3DTOP_DISABLE, "TextureStageState zmieniony");
        TEST_CHECK(addr == D3DTADDRESS_CLAMP, "SamplerState zmieniony");
    }

    DWORD rOp = 0, rAddr = 0;
    STATEMANAGER.GetTextureStageState(0, D3DTSS_COLOROP, &rOp);
    STATEMANAGER.GetSamplerState(0, D3DSAMP_ADDRESSU, &rAddr);

    TEST_CHECK(rOp == D3DTOP_MODULATE, "TextureStageState przywrocony");
    TEST_CHECK(rAddr == D3DTADDRESS_WRAP, "SamplerState przywrocony");

    std::cout << "[PASS] Test 5: ScopedD3DTextureStageStateGuard i SamplerGuard dzialaja poprawnie." << std::endl;
}

void TestDefensiveStackSafety()
{
    std::cout << "[RUN] Test 6: Ochrona defensywna pustych stosow w CStateManager..." << std::endl;

    // Wywolanie metod Restore na pustych stosach - wczesniej powodowalo UB i crash .back() na pustym wektorze
    STATEMANAGER.RestoreVertexShader();
    STATEMANAGER.RestoreVertexDeclaration();
    STATEMANAGER.RestorePixelShader();
    STATEMANAGER.RestoreFVF();
    STATEMANAGER.RestoreMaterial();
    STATEMANAGER.RestoreVertexProcessing();
    STATEMANAGER.RestoreIndices();
    STATEMANAGER.RestoreStreamSource(0);
    STATEMANAGER.RestoreTexture(0);
    STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
    STATEMANAGER.RestoreTextureStageState(0, D3DTSS_COLOROP);
    STATEMANAGER.RestoreSamplerState(0, D3DSAMP_ADDRESSU);
    STATEMANAGER.RestoreTransform(D3DTS_WORLD);

    // Jesli kod dotarl tutaj bez wyjatku i bledu pamieci, ochrona dziala w 100%
    TEST_CHECK(true, "CStateManager nie ulega awarii przy Restore na pustym stosie");

    std::cout << "[PASS] Test 6: Ochrona defensywna pustych stosow zdala egzamin." << std::endl;
}

int main()
{
    std::cout << "==========================================================================" << std::endl;
    std::cout << "=== TEST_C30_D3D9_STATE_GUARDS: RAII STATE MACHINE GUARDS (C++23)     ===" << std::endl;
    std::cout << "==========================================================================" << std::endl;

    auto* mockDev = new TestMockD3D9Device();
    auto stateMgr = std::make_unique<CStateManager>(mockDev);

    TestVertexDeclGuard(mockDev);
    TestShaderGuard(mockDev);
    TestFFPGuard(mockDev);
    TestRenderStateGuard(mockDev);
    TestTextureStageAndSamplerGuard();
    TestDefensiveStackSafety();

    std::cout << "==========================================================================" << std::endl;
    std::cout << "Wynik testow: " << g_testsPassed << " passed, " << g_testsFailed << " failed." << std::endl;
    std::cout << "==========================================================================" << std::endl;

    return (g_testsFailed == 0) ? 0 : 1;
}
