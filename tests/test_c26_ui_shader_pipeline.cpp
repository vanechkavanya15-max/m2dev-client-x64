#include <iostream>
#include <cmath>
#include <cstring>

// --- Mocks and Stubs for Windows / D3D9 ---

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;
typedef D3DMATRIX D3DXMATRIX;

typedef void* LPDIRECT3DDEVICE9;

#define D3DTS_PROJECTION 2
#define D3DTS_VIEW 3

#define D3DRS_ALPHABLENDENABLE 27
#define D3DRS_SRCBLEND 19
#define D3DRS_DESTBLEND 20
#define D3DRS_ZENABLE 7
#define D3DRS_ZWRITEENABLE 14
#define D3DRS_CULLMODE 22
#define D3DRS_LIGHTING 137
#define D3DRS_SRGBWRITEENABLE 194

#define D3DBLEND_SRCALPHA 5
#define D3DBLEND_INVSRCALPHA 6
#define D3DZB_FALSE 0
#define D3DCULL_NONE 1

#define D3DSAMP_SRGBTEXTURE 11
#define D3DSAMP_MINFILTER 5
#define D3DSAMP_MAGFILTER 6
#define D3DSAMP_MIPFILTER 7

#define D3DTEXF_NONE 0
#define D3DTEXF_LINEAR 2

#define TRUE 1
#define FALSE 0
typedef int BOOL;
typedef unsigned long DWORD;

// Mocks for D3DX Math Functions
void D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
    std::memset(pOut, 0, sizeof(D3DXMATRIX));
    pOut->_11 = 1.0f;
    pOut->_22 = 1.0f;
    pOut->_33 = 1.0f;
    pOut->_44 = 1.0f;
}

void D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, float l, float r, float b, float t, float zn, float zf)
{
    std::memset(pOut, 0, sizeof(D3DXMATRIX));
    pOut->_11 = 2.0f / (r - l);
    pOut->_22 = 2.0f / (t - b);
    pOut->_33 = 1.0f / (zf - zn);
    pOut->_41 = (l + r) / (l - r);
    pOut->_42 = (t + b) / (b - t);
    pOut->_43 = zn / (zn - zf);
    pOut->_44 = 1.0f;
}

// Mock StateManager
class MockStateManager
{
public:
    void SaveTransform(int Transform, const D3DXMATRIX* pMatrix) {}
    void RestoreTransform(int Transform) {}

    void SaveRenderState(int Type, int dwValue) {}
    void RestoreRenderState(int Type) {}

    void SaveSamplerState(int dwStage, int Type, int dwValue) {}
    void RestoreSamplerState(int dwStage, int Type) {}
};

MockStateManager g_MockStateManager;
#define STATEMANAGER g_MockStateManager

// --- Include Target Source ---
// We will test UIShaderPipeline.cpp directly by including it here after mocking.
#ifndef STATEMANAGER
#define STATEMANAGER g_MockStateManager
#endif
#include "../src/EterLib/Render/UIShaderPipeline.cpp"

// --- Tests ---

void test_SetOrthoProjection()
{
    EterLib::Render::UIShaderPipeline pipeline;
    float width = 800.0f;
    float height = 600.0f;

    pipeline.SetOrthoProjection(width, height);
    const D3DXMATRIX& mat = pipeline.GetOrthoMatrix();

    // Verify matrix values against expected D3DXMatrixOrthoOffCenterLH calculations
    // for l=0, r=800, b=600, t=0, zn=0, zf=1
    
    // Expected:
    // _11 = 2 / 800 = 0.0025
    // _22 = 2 / (0 - 600) = -0.00333333
    // _33 = 1 / 1 = 1.0
    // _41 = (0 + 800) / (0 - 800) = -1.0
    // _42 = (0 + 600) / (600 - 0) = 1.0
    // _43 = 0 / (0 - 1) = 0.0
    // _44 = 1.0

    float expected_11 = 2.0f / width;
    float expected_22 = 2.0f / -height;
    float expected_33 = 1.0f;
    float expected_41 = -1.0f;
    float expected_42 = 1.0f;
    float expected_43 = 0.0f;
    float expected_44 = 1.0f;

    bool passed = true;
    if (std::abs(mat._11 - expected_11) > 1e-5) { std::cout << "_11 mismatch: " << mat._11 << " vs " << expected_11 << std::endl; passed = false; }
    if (std::abs(mat._22 - expected_22) > 1e-5) { std::cout << "_22 mismatch: " << mat._22 << " vs " << expected_22 << std::endl; passed = false; }
    if (std::abs(mat._33 - expected_33) > 1e-5) { std::cout << "_33 mismatch: " << mat._33 << " vs " << expected_33 << std::endl; passed = false; }
    if (std::abs(mat._41 - expected_41) > 1e-5) { std::cout << "_41 mismatch: " << mat._41 << " vs " << expected_41 << std::endl; passed = false; }
    if (std::abs(mat._42 - expected_42) > 1e-5) { std::cout << "_42 mismatch: " << mat._42 << " vs " << expected_42 << std::endl; passed = false; }
    if (std::abs(mat._43 - expected_43) > 1e-5) { std::cout << "_43 mismatch: " << mat._43 << " vs " << expected_43 << std::endl; passed = false; }
    if (std::abs(mat._44 - expected_44) > 1e-5) { std::cout << "_44 mismatch: " << mat._44 << " vs " << expected_44 << std::endl; passed = false; }

    if (passed)
    {
        std::cout << "test_SetOrthoProjection passed." << std::endl;
    }
    else
    {
        std::cout << "test_SetOrthoProjection failed." << std::endl;
        exit(1);
    }
}

int main()
{
    test_SetOrthoProjection();
    return 0;
}

