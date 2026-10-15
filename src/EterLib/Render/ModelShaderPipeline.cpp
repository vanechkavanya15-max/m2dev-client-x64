#include "ModelShaderPipeline.h"
#include <algorithm>

namespace EterLib::Render
{
    void ModelShaderPipeline::SetBonePalette(std::span<const D3DMATRIX> boneMatrices)
    {
        m_bonePalette.assign(boneMatrices.begin(), boneMatrices.end());
    }

    void ModelShaderPipeline::SetMaterial(float specularPower, D3DCOLORVALUE diffuse)
    {
        m_specularPower = specularPower;
        m_diffuse = diffuse;
    }

    void ModelShaderPipeline::Bind(LPDIRECT3DDEVICE9 dev)
    {
        if (!dev)
            return;

        // Assuming vertex shader registers:
        // c0-c3: Projection/View matrix (handled elsewhere typically, or here if needed)
        // Let's assume we bind material properties and bone palette starting from some index.
        // For example:
        // c4: diffuse color
        // c5: specular power (x component)
        // c10+: bone matrices (3 registers per bone for 4x3 or 4 registers per bone for 4x4)
        
        float diffuseArr[4] = {m_diffuse.r, m_diffuse.g, m_diffuse.b, m_diffuse.a};
        dev->SetVertexShaderConstantF(4, diffuseArr, 1);
        
        float specArr[4] = {m_specularPower, 0.0f, 0.0f, 0.0f};
        dev->SetVertexShaderConstantF(5, specArr, 1);

        if (!m_bonePalette.empty())
        {
            // Assuming 4 registers per bone matrix for simplicity (4x4 matrix).
            // A typical bone matrix array might be passed continuously.
            dev->SetVertexShaderConstantF(10, reinterpret_cast<const float*>(m_bonePalette.data()), static_cast<UINT>(m_bonePalette.size() * 4));
        }
    }
} // namespace EterLib::Render

