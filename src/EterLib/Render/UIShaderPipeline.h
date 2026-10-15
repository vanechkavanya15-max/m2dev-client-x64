#pragma once

#include <d3d9.h>
#include <d3dx9.h>

namespace EterLib::Render
{
    class UIShaderPipeline
    {
    public:
        UIShaderPipeline() = default;
        ~UIShaderPipeline() = default;

        UIShaderPipeline(const UIShaderPipeline&) = delete;
        UIShaderPipeline& operator=(const UIShaderPipeline&) = delete;
        UIShaderPipeline(UIShaderPipeline&&) = delete;
        UIShaderPipeline& operator=(UIShaderPipeline&&) = delete;

        void SetOrthoProjection(float width, float height);
        void Bind(LPDIRECT3DDEVICE9 dev, bool isTextured);
        void Unbind(LPDIRECT3DDEVICE9 dev) noexcept;

        const D3DXMATRIX& GetOrthoMatrix() const { return m_orthoMatrix; }

    private:
        D3DXMATRIX m_orthoMatrix;
        D3DXMATRIX m_identityMatrix;
        bool m_isTextured = false;
    };
}

