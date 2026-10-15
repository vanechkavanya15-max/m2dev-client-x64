#pragma once

#include "../StdAfx.h"
#include "../StateManager.h"
#include <cstdint>

namespace EterLib::Render
{
    /**
     * @brief RAII scope guard for managing DirectX 9 Vertex Shaders, Declarations, and FVF.
     * Restores previous state upon destruction. Zero-Conflict implementation.
     */
    class VertexShaderScope
    {
    public:
        /**
         * @brief Constructs scope and applies new shader state if provided.
         * 
         * @param shader The vertex shader to set (can be nullptr).
         * @param declaration The vertex declaration to set (can be nullptr).
         * @param fvf The Flexible Vertex Format to set (can be 0).
         */
        explicit VertexShaderScope(
            LPDIRECT3DVERTEXSHADER9 shader,
            LPDIRECT3DVERTEXDECLARATION9 declaration = nullptr,
            DWORD fvf = 0)
        {
            if (shader)
            {
                STATEMANAGER.SaveVertexShader(shader);
                m_hasShader = true;
            }
            
            if (declaration)
            {
                STATEMANAGER.SaveVertexDeclaration(declaration);
                m_hasDeclaration = true;
            }

            if (fvf != 0)
            {
                STATEMANAGER.SaveFVF(fvf);
                m_hasFvf = true;
            }
        }

        ~VertexShaderScope()
        {
            if (m_hasFvf)
            {
                STATEMANAGER.RestoreFVF();
            }

            if (m_hasDeclaration)
            {
                STATEMANAGER.RestoreVertexDeclaration();
            }
            
            if (m_hasShader)
            {
                STATEMANAGER.RestoreVertexShader();
            }
        }

        // Prevent copying and moving
        VertexShaderScope(const VertexShaderScope&) = delete;
        VertexShaderScope& operator=(const VertexShaderScope&) = delete;
        VertexShaderScope(VertexShaderScope&&) = delete;
        VertexShaderScope& operator=(VertexShaderScope&&) = delete;

    private:
        bool m_hasShader = false;
        bool m_hasDeclaration = false;
        bool m_hasFvf = false;
    };
}
