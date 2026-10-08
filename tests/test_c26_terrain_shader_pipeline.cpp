#define _D3D9_H_ 
#define __D3DX9_H__ // mock d3dx9 too

#include <doctest/doctest.h>
#include <map>
#include <vector>
#include <string>

typedef unsigned int UINT;
typedef float FLOAT;
typedef long HRESULT;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
#define D3D_OK 0
#define FAILED(hr) (((HRESULT)(hr)) < 0)

struct D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

struct D3DVECTOR {
    float x;
    float y;
    float z;
};

// IUnknown mock
struct IUnknown {
    virtual HRESULT QueryInterface(const void* riid, void** ppvObject) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
};

struct IDirect3DVertexShader9 : public IUnknown {
    HRESULT QueryInterface(const void* riid, void** ppvObject) override { return 0; }
    ULONG AddRef() override { return 1; }
    ULONG Release() override { return 0; }
};
struct IDirect3DPixelShader9 : public IUnknown {
    HRESULT QueryInterface(const void* riid, void** ppvObject) override { return 0; }
    ULONG AddRef() override { return 1; }
    ULONG Release() override { return 0; }
};

struct IDirect3DDevice9 {
    virtual ~IDirect3DDevice9() = default;
    virtual HRESULT SetVertexShader(IDirect3DVertexShader9* pShader) = 0;
    virtual HRESULT SetPixelShader(IDirect3DPixelShader9* pShader) = 0;
    virtual HRESULT SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader) = 0;
    virtual HRESULT CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader) = 0;
};

#define LPDIRECT3DDEVICE9 IDirect3DDevice9*

// ID3DXBuffer mock
struct ID3DXBuffer : public IUnknown {
    virtual void* GetBufferPointer() = 0;
    virtual DWORD GetBufferSize() = 0;
};

struct MockD3DXBuffer : public ID3DXBuffer {
    HRESULT QueryInterface(const void* riid, void** ppvObject) override { return 0; }
    ULONG AddRef() override { return 1; }
    ULONG Release() override { delete this; return 0; }
    void* GetBufferPointer() override { return buffer; }
    DWORD GetBufferSize() override { return 1; }
    DWORD buffer[1] = {0};
};

// Mock D3DXCompileShader
HRESULT D3DXCompileShader(
    const char* pSrcData,
    UINT srcDataLen,
    const void* pDefines,
    const void* pInclude,
    const char* pFunctionName,
    const char* pProfile,
    DWORD Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs,
    ID3DXBuffer** ppConstantTable) {
    if (ppShader) {
        *ppShader = new MockD3DXBuffer();
    }
    return 0; // D3D_OK
}

// Include CPP file directly for the test to get the mocked types
#include "../src/EterLib/Render/TerrainShaderPipeline.cpp"

class MockD3DDevice : public IDirect3DDevice9 {
public:
    std::map<UINT, std::vector<float>> vs_constants;
    std::map<UINT, std::vector<float>> ps_constants;
    IDirect3DVertexShader9* current_vs = nullptr;
    IDirect3DPixelShader9* current_ps = nullptr;
    bool vs_cleared = false;
    bool ps_cleared = false;
    IDirect3DVertexShader9 mock_vs;
    IDirect3DPixelShader9 mock_ps;

    HRESULT SetVertexShader(IDirect3DVertexShader9* pShader) override {
        current_vs = pShader;
        if (!pShader) vs_cleared = true;
        return D3D_OK;
    }

    HRESULT SetPixelShader(IDirect3DPixelShader9* pShader) override {
        current_ps = pShader;
        if (!pShader) ps_cleared = true;
        return D3D_OK;
    }

    HRESULT SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) override {
        std::vector<float> data(pConstantData, pConstantData + Vector4fCount * 4);
        vs_constants[StartRegister] = data;
        return D3D_OK;
    }

    HRESULT SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) override {
        std::vector<float> data(pConstantData, pConstantData + Vector4fCount * 4);
        ps_constants[StartRegister] = data;
        return D3D_OK;
    }

    HRESULT CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader) override {
        if (ppShader) *ppShader = &mock_vs;
        return D3D_OK;
    }

    HRESULT CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader) override {
        if (ppShader) *ppShader = &mock_ps;
        return D3D_OK;
    }
};

TEST_CASE("TerrainShaderPipeline Constant Binding") {
    MockD3DDevice mockDev;
    EterLib::Render::TerrainShaderPipeline pipeline;
    
    REQUIRE(pipeline.Initialize(&mockDev) == true);

    D3DMATRIX viewProj = {};
    viewProj._11 = 1.0f; viewProj._22 = 2.0f; viewProj._33 = 3.0f; viewProj._44 = 4.0f;

    D3DMATRIX world = {};
    world._11 = 5.0f; world._22 = 6.0f; world._33 = 7.0f; world._44 = 8.0f;

    D3DVECTOR lightDir = { 0.0f, -1.0f, 0.0f };

    pipeline.Bind(&mockDev, viewProj, world, lightDir);
    
    CHECK(mockDev.current_vs == &mockDev.mock_vs);
    CHECK(mockDev.current_ps == &mockDev.mock_ps);

    REQUIRE(mockDev.vs_constants.count(0) > 0);
    CHECK(mockDev.vs_constants[0][0] == 1.0f);
    CHECK(mockDev.vs_constants[0][5] == 2.0f);

    REQUIRE(mockDev.vs_constants.count(4) > 0);
    CHECK(mockDev.vs_constants[4][0] == 5.0f);

    REQUIRE(mockDev.ps_constants.count(0) > 0);
    CHECK(mockDev.ps_constants[0][0] == 0.0f);
    CHECK(mockDev.ps_constants[0][1] == -1.0f);
    CHECK(mockDev.ps_constants[0][2] == 0.0f);

    REQUIRE(mockDev.vs_constants.count(8) > 0);
    CHECK(mockDev.vs_constants[8][0] == 0.0f);
    CHECK(mockDev.vs_constants[8][1] == -1.0f);
    CHECK(mockDev.vs_constants[8][2] == 0.0f);

    pipeline.Unbind(&mockDev);
    CHECK(mockDev.vs_cleared == true);
    CHECK(mockDev.ps_cleared == true);
}

TEST_CASE("TerrainShaderPipeline Shaders Content") {
    // Basic string find to verify HLSL content was embedded
    std::string hlsl(EterLib::Render::g_terrainShaderHLSL);
    CHECK(hlsl.find("Multi-layer terrain blending (splatting up to 4 textures)") != std::string::npos);
    CHECK(hlsl.find("TexLayer0") != std::string::npos);
    CHECK(hlsl.find("TexLayer3") != std::string::npos);
    CHECK(hlsl.find("Phong") != std::string::npos); // Make sure phong shading is in there
}

