#pragma once

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#include <d3dx9.h>
#endif

namespace EterLib::Render
{
    class TerrainShadowMapProjector
    {
    public:
        TerrainShadowMapProjector() noexcept;
        ~TerrainShadowMapProjector() = default;

        void SetupSunMatrix(const D3DVECTOR& sunDir, const D3DVECTOR& cameraPos);
        D3DMATRIX GetLightViewProjMatrix() const noexcept;
        bool IsPatchInShadow(float x, float y, float size) const noexcept;

    private:
        D3DMATRIX m_lightViewProj;
        float m_sunRadius;
    };
}

