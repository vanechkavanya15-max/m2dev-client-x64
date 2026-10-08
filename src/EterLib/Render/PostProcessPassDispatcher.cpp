#include "PostProcessPassDispatcher.h"
#include <array>
#include "../StateManager.h"

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
        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(effectShader);
        
        D3DMATRIX identity;
        std::memset(&identity, 0, sizeof(identity));
        identity.m[0][0] = identity.m[1][1] = identity.m[2][2] = identity.m[3][3] = 1.0f;
        STATEMANAGER.SaveTransform(D3DTS_WORLD, (const D3DXMATRIX*)&identity);
        STATEMANAGER.SaveTransform(D3DTS_VIEW, (const D3DXMATRIX*)&identity);
        STATEMANAGER.SaveTransform(D3DTS_PROJECTION, (const D3DXMATRIX*)&identity);

        STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, FALSE);
        STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
        STATEMANAGER.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        STATEMANAGER.SaveRenderState(D3DRS_LIGHTING, FALSE);
        
        STATEMANAGER.SaveSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        STATEMANAGER.SaveSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        STATEMANAGER.SaveSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        STATEMANAGER.SaveSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        // NDC coordinates (-1..1) for full-screen quad using XYZ
        static const std::array<ScreenVertex, 4> vertices = {
            ScreenVertex{-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
            ScreenVertex{ 1.0f,  1.0f, 0.0f, 1.0f, 0.0f},
            ScreenVertex{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
            ScreenVertex{ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f}
        };

        dev->SetFVF(D3DFVF_SCREENVERTEX);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices.data(), sizeof(ScreenVertex));
        
        STATEMANAGER.RestoreSamplerState(0, D3DSAMP_ADDRESSV);
        STATEMANAGER.RestoreSamplerState(0, D3DSAMP_ADDRESSU);
        STATEMANAGER.RestoreSamplerState(0, D3DSAMP_MAGFILTER);
        STATEMANAGER.RestoreSamplerState(0, D3DSAMP_MINFILTER);

        STATEMANAGER.RestoreRenderState(D3DRS_LIGHTING);
        STATEMANAGER.RestoreRenderState(D3DRS_CULLMODE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);

        STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);
        STATEMANAGER.RestoreTransform(D3DTS_VIEW);
        STATEMANAGER.RestoreTransform(D3DTS_WORLD);

        dev->SetRenderTarget(0, oldRenderTarget);
        if (oldRenderTarget)
        {
            oldRenderTarget->Release();
        }
        
        dev->SetTexture(0, nullptr);
        dev->SetPixelShader(nullptr);
    }
}

