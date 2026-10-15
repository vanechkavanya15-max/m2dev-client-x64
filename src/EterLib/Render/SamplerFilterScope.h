#pragma once

#include <d3d9.h>
#include "StateManager.h"

namespace EterLib::Render
{
    class SamplerFilterScope
    {
    public:
        enum class Preset
        {
            Point,
            Linear,
            Anisotropic
        };

        SamplerFilterScope(DWORD dwStage, Preset preset, DWORD maxAnisotropy = 4)
            : m_dwStage(dwStage)
        {
            auto& stateManager = CStateManager::Instance();

            DWORD minFilter = D3DTEXF_POINT;
            DWORD magFilter = D3DTEXF_POINT;
            DWORD mipFilter = D3DTEXF_POINT;
            DWORD anisotropy = 1;

            switch (preset)
            {
            case Preset::Point:
                minFilter = D3DTEXF_POINT;
                magFilter = D3DTEXF_POINT;
                mipFilter = D3DTEXF_POINT;
                anisotropy = 1;
                break;
            case Preset::Linear:
                minFilter = D3DTEXF_LINEAR;
                magFilter = D3DTEXF_LINEAR;
                mipFilter = D3DTEXF_LINEAR;
                anisotropy = 1;
                break;
            case Preset::Anisotropic:
                minFilter = D3DTEXF_ANISOTROPIC;
                magFilter = D3DTEXF_ANISOTROPIC;
                mipFilter = D3DTEXF_LINEAR; // Generally mip is linear for anisotropic
                anisotropy = maxAnisotropy;
                break;
            }

            stateManager.SaveSamplerState(m_dwStage, D3DSAMP_MINFILTER, minFilter);
            stateManager.SaveSamplerState(m_dwStage, D3DSAMP_MAGFILTER, magFilter);
            stateManager.SaveSamplerState(m_dwStage, D3DSAMP_MIPFILTER, mipFilter);
            stateManager.SaveSamplerState(m_dwStage, D3DSAMP_MAXANISOTROPY, anisotropy);
        }

        ~SamplerFilterScope()
        {
            auto& stateManager = CStateManager::Instance();
            stateManager.RestoreSamplerState(m_dwStage, D3DSAMP_MAXANISOTROPY);
            stateManager.RestoreSamplerState(m_dwStage, D3DSAMP_MIPFILTER);
            stateManager.RestoreSamplerState(m_dwStage, D3DSAMP_MAGFILTER);
            stateManager.RestoreSamplerState(m_dwStage, D3DSAMP_MINFILTER);
        }

        // Prevent copying and assignment
        SamplerFilterScope(const SamplerFilterScope&) = delete;
        SamplerFilterScope& operator=(const SamplerFilterScope&) = delete;

    private:
        DWORD m_dwStage;
    };
}
