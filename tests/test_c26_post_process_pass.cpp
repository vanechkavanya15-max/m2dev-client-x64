#include "doctest.h"
#include <array>

// Ensure we mock what's needed for the test, but compile standalone
#define _D3D9_H_
#define __D3D9_H__

typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long HRESULT;

#define TRUE 1
#define FALSE 0

const int D3DRS_ALPHABLENDENABLE = 27;
const int D3DRS_ZENABLE = 7;
const int D3DRS_ZWRITEENABLE = 14;
const int D3DRS_CULLMODE = 22;
const int D3DCULL_NONE = 1;
const int D3DSAMP_MINFILTER = 1;
const int D3DSAMP_MAGFILTER = 2;
const int D3DSAMP_ADDRESSU = 3;
const int D3DSAMP_ADDRESSV = 4;
const int D3DTEXF_LINEAR = 2;
const int D3DTADDRESS_CLAMP = 3;
const int D3DPT_TRIANGLESTRIP = 5;

const DWORD D3DFVF_XYZ = 0x002;
const DWORD D3DFVF_TEX1 = 0x100;

struct IDirect3DSurface9 {
    virtual void Release() {}
};

struct IDirect3DTexture9 {};
struct IDirect3DPixelShader9 {};

struct IDirect3DDevice9 {
    virtual HRESULT GetRenderTarget(DWORD, IDirect3DSurface9**) { return 0; }
    virtual HRESULT SetRenderTarget(DWORD, IDirect3DSurface9*) { return 0; }
    virtual HRESULT SetTexture(DWORD, IDirect3DTexture9*) { return 0; }
    virtual HRESULT SetPixelShader(IDirect3DPixelShader9*) { return 0; }
    virtual HRESULT SetRenderState(int, DWORD) { return 0; }
    virtual HRESULT SetSamplerState(DWORD, int, DWORD) { return 0; }
    virtual HRESULT SetFVF(DWORD) { return 0; }
    virtual HRESULT DrawPrimitiveUP(int, UINT, const void*, UINT) { return 0; }
};

typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;
typedef IDirect3DTexture9* LPDIRECT3DTEXTURE9;
typedef IDirect3DSurface9* LPDIRECT3DSURFACE9;
typedef IDirect3DPixelShader9* LPDIRECT3DPIXELSHADER9;

// Directly include source to compile cleanly within doctest mock env
#include "../src/EterLib/Render/PostProcessPassDispatcher.cpp"

TEST_CASE("PostProcessPassDispatcher ScreenVertex Coordinates NDC (-1..1)")
{
    std::array<EterLib::Render::ScreenVertex, 4> vertices = {
        EterLib::Render::ScreenVertex{-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        EterLib::Render::ScreenVertex{ 1.0f,  1.0f, 0.0f, 1.0f, 0.0f},
        EterLib::Render::ScreenVertex{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        EterLib::Render::ScreenVertex{ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f}
    };
    
    CHECK(vertices[0].x == doctest::Approx(-1.0f));
    CHECK(vertices[0].y == doctest::Approx(1.0f));
    CHECK(vertices[0].z == doctest::Approx(0.0f));
    CHECK(vertices[0].u == doctest::Approx(0.0f));
    CHECK(vertices[0].v == doctest::Approx(0.0f));
    
    CHECK(vertices[3].x == doctest::Approx(1.0f));
    CHECK(vertices[3].y == doctest::Approx(-1.0f));
    CHECK(vertices[3].z == doctest::Approx(0.0f));
    CHECK(vertices[3].u == doctest::Approx(1.0f));
    CHECK(vertices[3].v == doctest::Approx(1.0f));
}

