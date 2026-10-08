#include "UIShaderPipeline.h"
#include "../StateManager.h"

namespace EterLib::Render
{
    void UIShaderPipeline::SetOrthoProjection(float width, float height)
    {
        D3DXMatrixOrthoOffCenterLH(&m_orthoMatrix, 0.0f, width, height, 0.0f, 0.0f, 1.0f);
        D3DXMatrixIdentity(&m_identityMatrix);
    }

    void UIShaderPipeline::Bind(LPDIRECT3DDEVICE9 dev, bool isTextured)
    {
        m_isTextured = isTextured;
        STATEMANAGER.SaveTransform(D3DTS_PROJECTION, &m_orthoMatrix);
        STATEMANAGER.SaveTransform(D3DTS_VIEW, &m_identityMatrix);

        STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

        STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);

        STATEMANAGER.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        STATEMANAGER.SaveRenderState(D3DRS_LIGHTING, FALSE);

        if (isTextured)
        {
            STATEMANAGER.SaveRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
            STATEMANAGER.SaveSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
            STATEMANAGER.SaveSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            STATEMANAGER.SaveSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            STATEMANAGER.SaveSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        }
    }

    void UIShaderPipeline::Unbind(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (m_isTextured)
        {
            STATEMANAGER.RestoreRenderState(D3DRS_SRGBWRITEENABLE);
            STATEMANAGER.RestoreSamplerState(0, D3DSAMP_SRGBTEXTURE);
            STATEMANAGER.RestoreSamplerState(0, D3DSAMP_MINFILTER);
            STATEMANAGER.RestoreSamplerState(0, D3DSAMP_MAGFILTER);
            STATEMANAGER.RestoreSamplerState(0, D3DSAMP_MIPFILTER);
        }

        STATEMANAGER.RestoreRenderState(D3DRS_LIGHTING);
        STATEMANAGER.RestoreRenderState(D3DRS_CULLMODE);

        STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);

        STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);
        STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);

        STATEMANAGER.RestoreTransform(D3DTS_VIEW);
        STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);
    }
}

