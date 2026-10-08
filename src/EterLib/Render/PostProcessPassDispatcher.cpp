#include "PostProcessPassDispatcher.h"
#include <array>

namespace EterLib::Render
{
    struct ScreenVertex
    {
        float x, y, z;
        float u, v;
    };
    constexpr DWORD D3DFVF_SCREENVERTEX = D3DFVF_XYZ | D3DFVF_TEX1;

    void PostProcessPassDispatcher::Execute(LPDIRECT3DDEVICE9 dev, LPDIRECT3DTEXTURE9 source, LPDIRECT3DSURFACE9 dest, LPDIRECT3DPIXELSHADER9 effectShader)
    {
        if (!dev || !source || !dest || !effectShader)
            return;
            
        LPDIRECT3DSURFACE9 oldRenderTarget = nullptr;
        dev->GetRenderTarget(0, &oldRenderTarget);
        
        dev->SetRenderTarget(0, dest);
        
        dev->SetTexture(0, source);
        dev->SetPixelShader(effectShader);
        
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        
        dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        // NDC coordinates (-1..1) for full-screen quad using XYZ
        static const std::array<ScreenVertex, 4> vertices = {
            ScreenVertex{-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
            ScreenVertex{ 1.0f,  1.0f, 0.0f, 1.0f, 0.0f},
            ScreenVertex{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
            ScreenVertex{ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f}
        };

        dev->SetFVF(D3DFVF_SCREENVERTEX);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices.data(), sizeof(ScreenVertex));
        
        dev->SetRenderTarget(0, oldRenderTarget);
        if (oldRenderTarget)
        {
            oldRenderTarget->Release();
        }
        
        dev->SetTexture(0, nullptr);
        dev->SetPixelShader(nullptr);
    }
}

