#pragma once

#include <d3d9.h>
#include "../StateManager.h"
#include <d3dx9.h>


namespace EterLib::Render
{
    class TextureStageColorScope
    {
    public:
        explicit TextureStageColorScope(DWORD stage) : m_stage(stage)
        {
            DWORD op = 0, arg1 = 0, arg2 = 0;
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, &op);
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, &arg1);
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, &arg2);

            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, op);
            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, arg1);
            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, arg2);
        }

        ~TextureStageColorScope()
        {
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP);
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1);
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2);
        }

        TextureStageColorScope(const TextureStageColorScope&) = delete;
        TextureStageColorScope& operator=(const TextureStageColorScope&) = delete;

        void Modulate()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_MODULATE);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, D3DTA_TEXTURE);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, D3DTA_CURRENT);
        }

        void SelectArg1()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, D3DTA_TEXTURE);
        }

        void Disable()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_DISABLE);
        }

    private:
        DWORD m_stage;
    };
}
