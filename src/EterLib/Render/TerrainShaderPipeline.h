#pragma once

#include <d3d9.h>
#include <string_view>

namespace EterLib::Render {

class TerrainShaderPipeline {
public:
    TerrainShaderPipeline() = default;
    ~TerrainShaderPipeline();

    // Prevent copy/move to safely manage D3D resources
    TerrainShaderPipeline(const TerrainShaderPipeline&) = delete;
    TerrainShaderPipeline& operator=(const TerrainShaderPipeline&) = delete;

    bool Initialize(LPDIRECT3DDEVICE9 dev);
    void Bind(LPDIRECT3DDEVICE9 dev, const D3DMATRIX& viewProj, const D3DMATRIX& world, const D3DVECTOR& lightDir);
    void Unbind(LPDIRECT3DDEVICE9 dev) noexcept;

private:
    IDirect3DVertexShader9* m_vertexShader = nullptr;
    IDirect3DPixelShader9* m_pixelShader = nullptr;
};

} // namespace EterLib::Render

