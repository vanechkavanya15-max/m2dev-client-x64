#include "RenderStateSnapshot.h"

// If we are in the real environment, we include StateManager.h
// We can use a preprocessor macro to avoid including it in tests since test provides the macro.
#ifndef IS_TESTING
#include "../StateManager.h"
#else
#include "StateManager.h"
#endif

namespace EterLib::Render
{
    void RenderStateSnapshot::Capture()
    {
        // 1. Capture Render States
        m_renderStates[0].first = D3DRS_ZENABLE;
        STATEMANAGER.GetRenderState(D3DRS_ZENABLE, &m_renderStates[0].second);

        m_renderStates[1].first = D3DRS_ZWRITEENABLE;
        STATEMANAGER.GetRenderState(D3DRS_ZWRITEENABLE, &m_renderStates[1].second);

        m_renderStates[2].first = D3DRS_ALPHABLENDENABLE;
        STATEMANAGER.GetRenderState(D3DRS_ALPHABLENDENABLE, &m_renderStates[2].second);

        m_renderStates[3].first = D3DRS_CULLMODE;
        STATEMANAGER.GetRenderState(D3DRS_CULLMODE, &m_renderStates[3].second);

        m_renderStates[4].first = D3DRS_LIGHTING;
        STATEMANAGER.GetRenderState(D3DRS_LIGHTING, &m_renderStates[4].second);

        m_renderStates[5].first = D3DRS_FOGENABLE;
        STATEMANAGER.GetRenderState(D3DRS_FOGENABLE, &m_renderStates[5].second);
        
        m_renderStates[6].first = D3DRS_SRCBLEND;
        STATEMANAGER.GetRenderState(D3DRS_SRCBLEND, &m_renderStates[6].second);

        m_renderStates[7].first = D3DRS_DESTBLEND;
        STATEMANAGER.GetRenderState(D3DRS_DESTBLEND, &m_renderStates[7].second);

        // 2. Capture Texture Stage States (Stage 0)
        m_textureStageStates[0].first = D3DTSS_COLOROP;
        STATEMANAGER.GetTextureStageState(0, D3DTSS_COLOROP, &m_textureStageStates[0].second);
        
        m_textureStageStates[1].first = D3DTSS_ALPHAOP;
        STATEMANAGER.GetTextureStageState(0, D3DTSS_ALPHAOP, &m_textureStageStates[1].second);

        // 3. Capture Sampler States (Stage 0)
        m_samplerStates[0].first = D3DSAMP_MINFILTER;
        STATEMANAGER.GetSamplerState(0, D3DSAMP_MINFILTER, &m_samplerStates[0].second);

        m_samplerStates[1].first = D3DSAMP_MAGFILTER;
        STATEMANAGER.GetSamplerState(0, D3DSAMP_MAGFILTER, &m_samplerStates[1].second);

        m_samplerStates[2].first = D3DSAMP_MIPFILTER;
        STATEMANAGER.GetSamplerState(0, D3DSAMP_MIPFILTER, &m_samplerStates[2].second);

        // 4. Capture Textures
        STATEMANAGER.GetTexture(0, &m_textures[0]);

        // 5. Capture Transforms
        m_transforms[0].first = D3DTS_WORLD;
        STATEMANAGER.GetTransform(D3DTS_WORLD, &m_transforms[0].second);

        m_transforms[1].first = D3DTS_VIEW;
        STATEMANAGER.GetTransform(D3DTS_VIEW, &m_transforms[1].second);

        m_transforms[2].first = D3DTS_PROJECTION;
        STATEMANAGER.GetTransform(D3DTS_PROJECTION, &m_transforms[2].second);

        m_isCaptured = true;
    }

    void RenderStateSnapshot::Apply() const
    {
        if (!m_isCaptured)
            return;

        // 1. Apply Transforms
        for (const auto& transformPair : m_transforms)
        {
            STATEMANAGER.SetTransform(transformPair.first, &transformPair.second);
        }

        // 2. Apply Textures
        STATEMANAGER.SetTexture(0, m_textures[0]);

        // 3. Apply Sampler States (Stage 0)
        for (const auto& samplerStatePair : m_samplerStates)
        {
            STATEMANAGER.SetSamplerState(0, samplerStatePair.first, samplerStatePair.second);
        }

        // 4. Apply Texture Stage States (Stage 0)
        for (const auto& texStageStatePair : m_textureStageStates)
        {
            STATEMANAGER.SetTextureStageState(0, texStageStatePair.first, texStageStatePair.second);
        }

        // 5. Apply Render States
        for (const auto& renderStatePair : m_renderStates)
        {
            STATEMANAGER.SetRenderState(renderStatePair.first, renderStatePair.second);
        }
    }
}
