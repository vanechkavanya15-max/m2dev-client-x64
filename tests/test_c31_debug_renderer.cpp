#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <windows.h>
#include <d3d9.h>
#include <vector>
#include <cmath>

#include "Client/Graphics/Debug/DebugVertex.h"
#include "Client/Graphics/Debug/DebugRenderer.h"
#include "Client/World/ActorRegistry.h"

using namespace Client::Graphics::Debug;
using namespace Client::World;

// ============================================================================
// Mock IDirect3DDevice9 dla testow automatycznych potoku renderowania
// ============================================================================
class MockD3D9Device final : public IDirect3DDevice9 {
public:
    ULONG m_refCount{1};

    DWORD m_currentFVF{0};
    DWORD m_renderStates[256]{};
    IDirect3DBaseTexture9* m_textureStage0{nullptr};

    struct DrawCallInfo {
        D3DPRIMITIVETYPE primType;
        UINT primCount;
        std::vector<DebugVertex> vertices;
        DWORD zEnableState;
    };

    std::vector<DrawCallInfo> m_drawCalls;

    MockD3D9Device() {
        for (int i = 0; i < 256; ++i) {
            m_renderStates[i] = 0;
        }
        m_renderStates[D3DRS_ZENABLE] = D3DZB_TRUE;
        m_renderStates[D3DRS_ZWRITEENABLE] = TRUE;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppvObj) override {
        if (!ppvObj) return E_FAIL;
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG c = --m_refCount;
        if (c == 0) delete this;
        return c;
    }

    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override {
        if (static_cast<size_t>(State) < 256) {
            m_renderStates[State] = Value;
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override {
        if (pValue && static_cast<size_t>(State) < 256) {
            *pValue = m_renderStates[State];
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override {
        m_currentFVF = FVF;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override {
        if (pFVF) *pFVF = m_currentFVF;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE SetTexture(DWORD, IDirect3DBaseTexture9* pTexture) override {
        m_textureStage0 = pTexture;
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTexture(DWORD, IDirect3DBaseTexture9** ppTexture) override {
        if (ppTexture) {
            *ppTexture = m_textureStage0;
            if (m_textureStage0) m_textureStage0->AddRef();
        }
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT PrimitiveCount,
        const void* pVertexStreamZeroData,
        UINT VertexStreamZeroStride) override {

        DrawCallInfo info;
        info.primType = PrimitiveType;
        info.primCount = PrimitiveCount;
        info.zEnableState = m_renderStates[D3DRS_ZENABLE];

        const auto* ptr = static_cast<const DebugVertex*>(pVertexStreamZeroData);
        const size_t vertexCount = PrimitiveCount * 2;
        info.vertices.assign(ptr, ptr + vertexCount);

        m_drawCalls.push_back(std::move(info));
        return D3D_OK;
    }

    // Dummy implementacja pozostalych metod interfejsu COM
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetAvailableTextureMem() override { return 1024 * 1024; }
    HRESULT STDMETHODCALLTYPE EvictManagedResources() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT, D3DDISPLAYMODE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT, UINT, IDirect3DSurface9*) override { return D3D_OK; }
    void STDMETHODCALLTYPE SetCursorPosition(int, int, DWORD) override {}
    BOOL STDMETHODCALLTYPE ShowCursor(BOOL) override { return TRUE; }
    HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetSwapChain(UINT, IDirect3DSwapChain9**) override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetNumberOfSwapChains() override { return 1; }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE Present(const RECT*, const RECT*, HWND, const RGNDATA*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT, D3DRASTER_STATUS*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL) override { return D3D_OK; }
    void STDMETHODCALLTYPE SetGammaRamp(UINT, DWORD, const D3DGAMMARAMP*) override {}
    void STDMETHODCALLTYPE GetGammaRamp(UINT, D3DGAMMARAMP*) override {}
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DVolumeTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DCubeTexture9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT, DWORD, DWORD, D3DPOOL, IDirect3DVertexBuffer9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DIndexBuffer9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const POINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9*, IDirect3DBaseTexture9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const RECT*, D3DTEXTUREFILTERTYPE) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9*, const RECT*, D3DCOLOR) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9**, HANDLE*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD, IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD, IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE BeginScene() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE EndScene() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE Clear(DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE, D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetLight(DWORD, const D3DLIGHT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetLight(DWORD, D3DLIGHT9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD, BOOL) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD, BOOL*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD, const float*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD, float*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE, IDirect3DStateBlock9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE BeginStateBlock() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD, D3DSAMPLERSTATETYPE, DWORD*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT, const PALETTEENTRY*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT, PALETTEENTRY*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetScissorRect(const RECT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetScissorRect(RECT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL) override { return D3D_OK; }
    BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override { return FALSE; }
    HRESULT STDMETHODCALLTYPE SetNPatchMode(float) override { return D3D_OK; }
    float STDMETHODCALLTYPE GetNPatchMode() override { return 0.0f; }
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE ProcessVertices(UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const D3DVERTEXELEMENT9*, IDirect3DVertexDeclaration9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT, IDirect3DVertexBuffer9*, UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetStreamSource(UINT, IDirect3DVertexBuffer9**, UINT*, UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT, UINT*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD*, IDirect3DVertexShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT, const float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT, float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT, const int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT, int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT, const BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT, BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD*, IDirect3DPixelShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9**) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT, const float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT, float*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT, const int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT, int*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT, const BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT, BOOL*, UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT, const float*, const D3DRECTPATCH_INFO*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT, const float*, const D3DTRIPATCH_INFO*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DeletePatch(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE, IDirect3DQuery9**) override { return D3D_OK; }
};

TEST_CASE("DebugVertex - pamiec i format FVF") {
    CHECK(sizeof(DebugVertex) == 16);
    CHECK(DebugVertex::FVF == (D3DFVF_XYZ | D3DFVF_DIFFUSE));

    DebugVertex v{1.0f, 2.0f, 3.0f, 0xFF00FF00};
    CHECK(v.x == 1.0f);
    CHECK(v.y == 2.0f);
    CHECK(v.z == 3.0f);
    CHECK(v.color == 0xFF00FF00);
}

TEST_CASE("DebugRenderer - Singleton i operacje na prymitywach") {
    auto& renderer = DebugRenderer::GetInstance();
    CHECK(&renderer == DebugRenderer::InstancePtr());
    CHECK(&renderer == &DebugRenderer::Instance());

    renderer.Clear();
    CHECK(renderer.GetLineCount() == 0);
    CHECK(renderer.GetBoxCount() == 0);
    CHECK(renderer.GetCircleCount() == 0);

    SUBCASE("Dodawanie odcinkow linii") {
        renderer.AddLine(0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 100.0f, 0xFFFF0000, false);
        renderer.AddLine(10.0f, 10.0f, 10.0f, 50.0f, 50.0f, 50.0f, 0xFF00FF00, true);
        CHECK(renderer.GetLineCount() == 2);
    }

    SUBCASE("Dodawanie prostopadloscianow (Box)") {
        renderer.AddBox(-10.0f, -10.0f, 0.0f, 10.0f, 10.0f, 20.0f, 0xFF0000FF, false);
        CHECK(renderer.GetBoxCount() == 1);
    }

    SUBCASE("Dodawanie okregow (Circle)") {
        renderer.AddCircle(500.0f, 500.0f, 0.0f, 250.0f, 0xFFFFFF00, false);
        CHECK(renderer.GetCircleCount() == 1);
    }

    SUBCASE("Metoda Clear czysci wszystkie rejestry") {
        renderer.AddLine(0,0,0, 1,1,1, 0xFFFFFFFF);
        renderer.AddBox(0,0,0, 2,2,2, 0xFFFFFFFF);
        renderer.AddCircle(0,0,0, 10, 0xFFFFFFFF);
        renderer.AddTracer(100, 0xFFFF0000);
        renderer.AddActorBox(100, 0xFF00FF00);

        CHECK(renderer.GetLineCount() >= 2);
        CHECK(renderer.GetBoxCount() >= 2);

        renderer.Clear();
        CHECK(renderer.GetLineCount() == 0);
        CHECK(renderer.GetBoxCount() == 0);
        CHECK(renderer.GetCircleCount() == 0);
    }
}

TEST_CASE("DebugRenderer - Integracja z ActorRegistry oraz NEW_GetPixelPosition") {
    auto& renderer = DebugRenderer::GetInstance();
    renderer.Clear();

    ActorRegistry registry;
    EntityVid mainVid{1001};
    EntityVid targetVid{2002};

    ActorRecord mainRecord{
        .vid = mainVid,
        .race = 0,
        .type = 0,
        .x = 1000.0f,
        .y = 2000.0f,
        .z = 50.0f,
        .rotation = 0.0f,
        .name = "Hero",
        .guildId = 0,
        .empire = 1,
        .isDead = false
    };

    ActorRecord targetRecord{
        .vid = targetVid,
        .race = 101,
        .type = 1,
        .x = 1500.0f,
        .y = 2500.0f,
        .z = 60.0f,
        .rotation = 90.0f,
        .name = "WildDog",
        .guildId = 0,
        .empire = 0,
        .isDead = false
    };

    registry.RegisterActor(mainRecord);
    registry.RegisterActor(targetRecord);
    registry.SetMainActorVid(mainVid);

    renderer.SetActorRegistry(&registry);

    SUBCASE("Pobieranie pozycji z ActorRegistry") {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        CHECK(renderer.GetActorPosition(2002, x, y, z) == true);
        CHECK(x == 1500.0f);
        CHECK(y == 2500.0f);
        CHECK(z == 60.0f);

        CHECK(renderer.GetMainActorPosition(x, y, z) == true);
        CHECK(x == 1000.0f);
        CHECK(y == 2000.0f);
        CHECK(z == 50.0f);
    }

    SUBCASE("Zewnetrzny dostawca adaptera (np. symulacja CPythonCharacterManager::NEW_GetPixelPosition)") {
        renderer.SetPositionProviders(
            [](uint32_t vid, float& x, float& y, float& z) {
                if (vid == 777) {
                    x = 999.0f; y = 888.0f; z = 777.0f;
                    return true;
                }
                return false;
            },
            [](float& x, float& y, float& z) {
                x = 100.0f; y = 200.0f; z = 300.0f;
                return true;
            }
        );

        float x = 0.0f, y = 0.0f, z = 0.0f;
        CHECK(renderer.GetActorPosition(777, x, y, z) == true);
        CHECK(x == 999.0f);
        CHECK(renderer.GetMainActorPosition(x, y, z) == true);
        CHECK(x == 100.0f);

        // Reset providerow do domyslnych
        renderer.SetPositionProviders(nullptr, nullptr);
    }

    renderer.SetActorRegistry(nullptr);
}

TEST_CASE("DebugRenderer - Renderowanie Direct3D 9 i obsluga X-Ray") {
    auto& renderer = DebugRenderer::GetInstance();
    renderer.Clear();

    ActorRegistry registry;
    EntityVid mainVid{1};
    EntityVid enemyVid{2};

    registry.RegisterActor(ActorRecord{
        .vid = mainVid, .race = 0, .type = 0,
        .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .rotation = 0.0f, .name = "Player", .guildId = 0, .empire = 1, .isDead = false
    });
    registry.RegisterActor(ActorRecord{
        .vid = enemyVid, .race = 101, .type = 1,
        .x = 200.0f, .y = 300.0f, .z = 0.0f,
        .rotation = 0.0f, .name = "Enemy", .guildId = 0, .empire = 0, .isDead = false
    });
    registry.SetMainActorVid(mainVid);
    renderer.SetActorRegistry(&registry);

    // Dodajemy linie zwykla i linie X-Ray
    renderer.AddLine(0, 0, 0, 100, 100, 0, 0xFF00FF00, false);
    renderer.AddLine(10, 10, 0, 20, 20, 0, 0xFFFF0000, true);

    // Dodajemy tracer (xray = true)
    renderer.AddTracer(2, 0xFF00FFFF, true);

    // Dodajemy box aktora (xray = false)
    renderer.AddActorBox(2, 0xFFFFFF00, false);

    MockD3D9Device device;
    renderer.Render(&device);

    // Weryfikacja wykonania DrawCalls
    REQUIRE(device.m_drawCalls.size() >= 2);

    bool hasNormalDraw = false;
    bool hasXRayDraw = false;

    for (const auto& call : device.m_drawCalls) {
        CHECK(call.primType == D3DPT_LINELIST);
        CHECK(call.primCount > 0);
        CHECK(call.vertices.size() == call.primCount * 2);

        if (call.zEnableState == D3DZB_TRUE) {
            hasNormalDraw = true;
        } else if (call.zEnableState == FALSE) {
            hasXRayDraw = true;
        }
    }

    CHECK(hasNormalDraw == true);
    CHECK(hasXRayDraw == true);

    renderer.SetActorRegistry(nullptr);
    renderer.Clear();
}
