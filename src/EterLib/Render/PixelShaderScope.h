#pragma once

#include <d3d9.h>
#include "../StateManager.h"

namespace EterLib::Render
{
    class PixelShaderScope
    {
    public:
        explicit PixelShaderScope(LPDIRECT3DPIXELSHADER9 newShader)
            : m_originalShader(nullptr), m_stateChanged(false)
        {
            LPDIRECT3DPIXELSHADER9 currentShader = nullptr;
            STATEMANAGER.GetPixelShader(&currentShader);

            if (currentShader != newShader)
            {
                m_originalShader = currentShader;
                m_stateChanged = true;
                STATEMANAGER.SetPixelShader(newShader);
            }
        }

        ~PixelShaderScope()
        {
            if (m_stateChanged)
            {
                STATEMANAGER.SetPixelShader(m_originalShader);
            }
        }

        PixelShaderScope(const PixelShaderScope&) = delete;
        PixelShaderScope& operator=(const PixelShaderScope&) = delete;
        PixelShaderScope(PixelShaderScope&&) = delete;
        PixelShaderScope& operator=(PixelShaderScope&&) = delete;

    private:
        LPDIRECT3DPIXELSHADER9 m_originalShader;
        bool m_stateChanged;
    };
}
