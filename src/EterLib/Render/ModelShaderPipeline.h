#pragma once

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#include <d3dx9.h>
#endif

#include <span>
#include <vector>

namespace EterLib::Render
{
    class ModelShaderPipeline
    {
    public:
        ModelShaderPipeline() = default;
        ~ModelShaderPipeline() = default;

        void SetBonePalette(std::span<const D3DMATRIX> boneMatrices);
        void SetMaterial(float specularPower, D3DCOLORVALUE diffuse);
        void Bind(LPDIRECT3DDEVICE9 dev);

    private:
        std::vector<D3DMATRIX> m_bonePalette;
        float m_specularPower = 1.0f;
        D3DCOLORVALUE m_diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
    };
} // namespace EterLib::Render

