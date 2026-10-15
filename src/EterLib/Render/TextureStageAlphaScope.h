#pragma once

#include "../StdAfx.h"
#include "../StateManager.h"
#include <cstdint>

namespace EterLib::Render
{
    class TextureStageAlphaScope
    {
    public:
        [[nodiscard]] TextureStageAlphaScope(uint32_t stage, uint32_t alphaOp, uint32_t alphaArg1, uint32_t alphaArg2)
            : m_stage(stage)
        {
            if (m_stage > 7)
            {
                return;
            }

            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAOP, alphaOp);
            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAARG1, alphaArg1);
            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAARG2, alphaArg2);
            m_valid = true;
        }

        ~TextureStageAlphaScope()
        {
            if (m_valid)
            {
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAOP);
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAARG1);
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAARG2);
            }
        }

        TextureStageAlphaScope(const TextureStageAlphaScope&) = delete;
        TextureStageAlphaScope& operator=(const TextureStageAlphaScope&) = delete;
        TextureStageAlphaScope(TextureStageAlphaScope&&) = delete;
        TextureStageAlphaScope& operator=(TextureStageAlphaScope&&) = delete;

    private:
        uint32_t m_stage;
        bool m_valid = false;
    };
}
