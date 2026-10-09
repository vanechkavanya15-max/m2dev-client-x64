#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <memory>
#include <span>
#include <string_view>

#ifndef TEST_MODE_DISABLE_STDAFX
#include <d3d9.h>
#else
// Stub definitions for headless cross-compilation environments without DirectX SDK
typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef long HRESULT;
typedef unsigned int UINT;
typedef int BOOL;
typedef unsigned char BYTE;
typedef struct _GUID { unsigned long Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; } GUID;
typedef GUID IID;
typedef const GUID& REFIID;
typedef const GUID& REFGUID;
#define S_OK 0
#define E_NOINTERFACE 0x80004002L
#define E_NOTIMPL 0x80004001L
#define E_FAIL 0x80004005L
#define D3D_OK 0
#define D3DERR_INVALIDCALL 0x8876086CL
#define STDMETHODCALLTYPE __stdcall

typedef enum _D3DFORMAT { D3DFMT_UNKNOWN = 0, D3DFMT_A8R8G8B8 = 21, D3DFMT_X8R8G8B8 = 22 } D3DFORMAT;
typedef enum _D3DPOOL { D3DPOOL_DEFAULT = 0, D3DPOOL_MANAGED = 1, D3DPOOL_SYSTEMMEM = 2 } D3DPOOL;
typedef enum _D3DPRIMITIVETYPE { D3DPT_POINTLIST = 1, D3DPT_LINELIST = 2, D3DPT_LINESTRIP = 3, D3DPT_TRIANGLELIST = 4, D3DPT_TRIANGLESTRIP = 5, D3DPT_TRIANGLEFAN = 6 } D3DPRIMITIVETYPE;
typedef enum _D3DRENDERSTATETYPE { D3DRS_ZENABLE = 7, D3DRS_FILLMODE = 8, D3DRS_SHADEMODE = 9, D3DRS_ALPHABLENDENABLE = 27 } D3DRENDERSTATETYPE;
typedef enum _D3DTRANSFORMSTATETYPE { D3DTS_VIEW = 2, D3DTS_PROJECTION = 3, D3DTS_TEXTURE0 = 16 } D3DTRANSFORMSTATETYPE;
typedef enum _D3DSAMPLERSTATETYPE { D3DSAMP_ADDRESSU = 1, D3DSAMP_MAGFILTER = 5, D3DSAMP_MINFILTER = 6 } D3DSAMPLERSTATETYPE;
typedef enum _D3DTEXTURESTAGESTATETYPE { D3DTSS_COLOROP = 1, D3DTSS_COLORARG1 = 2 } D3DTEXTURESTAGESTATETYPE;
typedef enum _D3DRESOURCETYPE { D3DRTYPE_SURFACE = 1, D3DRTYPE_VOLUME = 2, D3DRTYPE_TEXTURE = 3, D3DRTYPE_VERTEXBUFFER = 5, D3DRTYPE_INDEXBUFFER = 6 } D3DRESOURCETYPE;
typedef enum _D3DSWAPEFFECT { D3DSWAPEFFECT_DISCARD = 1 } D3DSWAPEFFECT;

typedef struct _D3DMATRIX { float m[4][4]; } D3DMATRIX;
typedef struct _D3DVIEWPORT9 { DWORD X; DWORD Y; DWORD Width; DWORD Height; float MinZ; float MaxZ; } D3DVIEWPORT9;
typedef struct _D3DRECT { long x1; long y1; long x2; long y2; } D3DRECT;
typedef struct _D3DCOLORVALUE { float r, g, b, a; } D3DCOLORVALUE;
typedef struct _D3DMATERIAL9 { D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive; float Power; } D3DMATERIAL9;
typedef struct _D3DLIGHT9 { DWORD Type; D3DCOLORVALUE Diffuse; } D3DLIGHT9;
typedef struct _D3DCAPS9 { DWORD DeviceType; } D3DCAPS9;
typedef struct _D3DDISPLAYMODE { UINT Width, Height, RefreshRate; D3DFORMAT Format; } D3DDISPLAYMODE;
typedef struct _D3DDEVICE_CREATION_PARAMETERS { UINT AdapterOrdinal; DWORD DeviceType; void* hFocusWindow; DWORD BehaviorFlags; } D3DDEVICE_CREATION_PARAMETERS;
typedef struct _D3DPRESENT_PARAMETERS { UINT BackBufferWidth, BackBufferHeight; D3DFORMAT BackBufferFormat; UINT BackBufferCount; DWORD MultiSampleType, MultiSampleQuality; D3DSWAPEFFECT SwapEffect; void* hDeviceWindow; BOOL Windowed, EnableAutoDepthStencil; D3DFORMAT AutoDepthStencilFormat; DWORD Flags; UINT FullScreen_RefreshRateInHz, PresentationInterval; } D3DPRESENT_PARAMETERS;
typedef struct _D3DGAMMARAMP { unsigned short red[256]; unsigned short green[256]; unsigned short blue[256]; } D3DGAMMARAMP;
typedef struct _D3DRASTER_STATUS { BOOL InVBlank; UINT ScanLine; } D3DRASTER_STATUS;
typedef DWORD D3DCOLOR;
typedef void* HWND;

struct IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual ~IUnknown() = default;
};

struct IDirect3D9 : public IUnknown {};
struct IDirect3DResource9 : public IUnknown {};
struct IDirect3DBaseTexture9 : public IDirect3DResource9 {};
struct IDirect3DTexture9 : public IDirect3DBaseTexture9 {};
struct IDirect3DVolumeTexture9 : public IDirect3DBaseTexture9 {};
struct IDirect3DCubeTexture9 : public IDirect3DBaseTexture9 {};
struct IDirect3DVertexBuffer9 : public IDirect3DResource9 {};
struct IDirect3DIndexBuffer9 : public IDirect3DResource9 {};
struct IDirect3DSurface9 : public IDirect3DResource9 {};
struct IDirect3DSwapChain9 : public IUnknown {};
struct IDirect3DStateBlock9 : public IUnknown {};
struct IDirect3DVertexDeclaration9 : public IUnknown {};
struct IDirect3DVertexShader9 : public IUnknown {};
struct IDirect3DPixelShader9 : public IUnknown {};
struct IDirect3DQuery9 : public IUnknown {};

struct IDirect3DDevice9 : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel() = 0;
    virtual UINT STDMETHODCALLTYPE GetAvailableTextureMem() = 0;
    virtual HRESULT STDMETHODCALLTYPE EvictManagedResources() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** ppD3D9) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) = 0;
    virtual void STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) = 0;
    virtual BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) = 0;
    virtual UINT STDMETHODCALLTYPE GetNumberOfSwapChains() = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present(const D3DRECT* pSourceRect, const D3DRECT* pDestRect, HWND hDestWindowOverride, const void* pDirtyRegion) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, DWORD Type, IDirect3DSurface9** ppBackBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL bEnableDialogs) = 0;
    virtual void STDMETHODCALLTYPE SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp) = 0;
    virtual void STDMETHODCALLTYPE GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9* pSourceSurface, const D3DRECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, const void* pDestPoint) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9* pSourceSurface, const D3DRECT* pSourceRect, IDirect3DSurface9* pDestSurface, const D3DRECT* pDestRect, DWORD Filter) = 0;
    virtual HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9* pSurface, const D3DRECT* pRect, D3DCOLOR color) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, void** pSharedHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* pZStencilSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLight(DWORD Index, const D3DLIGHT9* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT9* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index, const float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateStateBlock(DWORD Type, IDirect3DStateBlock9** ppSB) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginStateBlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const void* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipStatus(void* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT PaletteNumber, const void* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber, void* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT *PaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetScissorRect(const void* pRect) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetScissorRect(void* pRect) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL bSoftware) = 0;
    virtual BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments) = 0;
    virtual float STDMETHODCALLTYPE GetNPatchMode() = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, int BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const void* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber, UINT Setting) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT Handle, const float* pNumSegs, const void* pRectPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT Handle, const float* pNumSegs, const void* pTriPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateQuery(DWORD Type, IDirect3DQuery9** ppQuery) = 0;
};
#endif

namespace EterLib::NullRHI
{
    /**
     * @struct NullD3D9Stats
     * @brief Liczniki operacji wirtualnego urzadzenia graficznego dla testow headless.
     */
    struct NullD3D9Stats
    {
        uint64_t drawCalls{0};
        uint64_t drawIndexedCalls{0};
        uint64_t drawUPCalls{0};
        uint64_t beginSceneCalls{0};
        uint64_t endSceneCalls{0};
        uint64_t presentCalls{0};
        uint64_t clearCalls{0};
        uint64_t setTextureCalls{0};
        uint64_t setRenderStateCalls{0};
    };

    /**
     * @class NullDirect3DDevice9
     * @brief Wirtualna atrapa interfejsu IDirect3DDevice9 dla srodowiska testow Headless i CI.
     * 
     * Wszystkie wywolania D3D zwracaja D3D_OK / S_OK bez renderowania fizycznych pikseli
     * i bez tworzenia natywnego okna Win32 HWND. Zbiera dokladne statystyki wywolan.
     */
    class NullDirect3DDevice9 final : public IDirect3DDevice9
    {
    public:
        NullDirect3DDevice9() noexcept;
        ~NullDirect3DDevice9() override = default;

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        // Statystyki testowe
        [[nodiscard]] const NullD3D9Stats& GetStats() const noexcept { return m_stats; }
        void ResetStats() noexcept { m_stats = {}; }

        // IDirect3DDevice9 metody kluczowe
        HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;
        UINT STDMETHODCALLTYPE GetAvailableTextureMem() override;
        HRESULT STDMETHODCALLTYPE EvictManagedResources() override;
        HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** ppD3D9) override;
        HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) override;
        HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) override;
        HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) override;
        HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override;
        void STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) override;
        BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow) override;
        HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) override;
        HRESULT STDMETHODCALLTYPE GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override;
        UINT STDMETHODCALLTYPE GetNumberOfSwapChains() override;
        HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;
        HRESULT STDMETHODCALLTYPE Present(const D3DRECT* pSourceRect, const D3DRECT* pDestRect, HWND hDestWindowOverride, const void* pDirtyRegion) override;
        HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, DWORD Type, IDirect3DSurface9** ppBackBuffer) override;
        HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override;
        HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL bEnableDialogs) override;
        void STDMETHODCALLTYPE SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp) override;
        void STDMETHODCALLTYPE GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) override;
        HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9* pSourceSurface, const D3DRECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, const void* pDestPoint) override;
        HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) override;
        HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) override;
        HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) override;
        HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9* pSourceSurface, const D3DRECT* pSourceRect, IDirect3DSurface9* pDestSurface, const D3DRECT* pDestRect, DWORD Filter) override;
        HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9* pSurface, const D3DRECT* pRect, D3DCOLOR color) override;
        HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, void** pSharedHandle) override;
        HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) override;
        HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) override;
        HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* pZStencilSurface) override;
        HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) override;
        HRESULT STDMETHODCALLTYPE BeginScene() override;
        HRESULT STDMETHODCALLTYPE EndScene() override;
        HRESULT STDMETHODCALLTYPE Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override;
        HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) override;
        HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
        HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) override;
        HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9* pViewport) override;
        HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) override;
        HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9* pMaterial) override;
        HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) override;
        HRESULT STDMETHODCALLTYPE SetLight(DWORD Index, const D3DLIGHT9* pLight) override;
        HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT9* pLight) override;
        HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) override;
        HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) override;
        HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index, const float* pPlane) override;
        HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) override;
        HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override;
        HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override;
        HRESULT STDMETHODCALLTYPE CreateStateBlock(DWORD Type, IDirect3DStateBlock9** ppSB) override;
        HRESULT STDMETHODCALLTYPE BeginStateBlock() override;
        HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) override;
        HRESULT STDMETHODCALLTYPE SetClipStatus(const void* pClipStatus) override;
        HRESULT STDMETHODCALLTYPE GetClipStatus(void* pClipStatus) override;
        HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) override;
        HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) override;
        HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override;
        HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override;
        HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override;
        HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override;
        HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) override;
        HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT PaletteNumber, const void* pEntries) override;
        HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber, void* pEntries) override;
        HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) override;
        HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT *PaletteNumber) override;
        HRESULT STDMETHODCALLTYPE SetScissorRect(CONST RECT* pRect) override;
        HRESULT STDMETHODCALLTYPE GetScissorRect(RECT* pRect) override;
        HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL bSoftware) override;
        BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override;
        HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments) override;
        float STDMETHODCALLTYPE GetNPatchMode() override;
        HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
        HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, int BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
        HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
        HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
        HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override;
        HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) override;
        HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override;
        HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override;
        HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override;
        HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override;
        HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader) override;
        HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) override;
        HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) override;
        HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) override;
        HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
        HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) override;
        HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
        HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) override;
        HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
        HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override;
        HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) override;
        HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber, UINT Setting) override;
        HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) override;
        HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) override;
        HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) override;
        HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader) override;
        HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) override;
        HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) override;
        HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) override;
        HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
        HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) override;
        HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
        HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) override;
        HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
        HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT Handle, const float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) override;
        HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT Handle, const float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) override;
        HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) override;
        HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override;

    private:
        std::atomic<ULONG> m_refCount{1};
        NullD3D9Stats m_stats{};
        D3DVIEWPORT9 m_viewport{0, 0, 1024, 768, 0.0f, 1.0f};
    };
}
