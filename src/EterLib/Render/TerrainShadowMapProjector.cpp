#include "TerrainShadowMapProjector.h"
#include <cmath>

namespace EterLib::Render
{
    TerrainShadowMapProjector::TerrainShadowMapProjector() noexcept
        : m_sunRadius(5000.0f)
    {
        D3DXMatrixIdentity(&m_lightViewProj);
    }

    void TerrainShadowMapProjector::SetupSunMatrix(const D3DVECTOR& sunDir, const D3DVECTOR& cameraPos)
    {
        D3DXVECTOR3 vSunDir(sunDir.x, sunDir.y, sunDir.z);
        D3DXVec3Normalize(&vSunDir, &vSunDir);

        D3DXVECTOR3 vTarget(cameraPos.x, cameraPos.y, cameraPos.z);
        float distance = 10000.0f;
        D3DXVECTOR3 vEye = vTarget - vSunDir * distance;

        D3DXVECTOR3 vUp(0.0f, 0.0f, 1.0f);
        if (std::abs(vSunDir.z) > 0.999f)
        {
            vUp.y = 1.0f;
            vUp.z = 0.0f;
        }

        D3DXMATRIX viewMatrix;
        D3DXMatrixLookAtLH(&viewMatrix, &vEye, &vTarget, &vUp);

        D3DXMATRIX projMatrix;
        D3DXMatrixOrthoLH(&projMatrix, m_sunRadius * 2.0f, m_sunRadius * 2.0f, 1.0f, distance * 2.0f);

        D3DXMatrixMultiply(&m_lightViewProj, &viewMatrix, &projMatrix);
    }

    D3DXMATRIX TerrainShadowMapProjector::GetLightViewProjMatrix() const noexcept
    {
        return m_lightViewProj;
    }

    bool TerrainShadowMapProjector::IsPatchInShadow(float x, float y, float size) const noexcept
    {
        D3DVECTOR corners[4] = {
            { x, y, 0.0f },
            { x + size, y, 0.0f },
            { x, y + size, 0.0f },
            { x + size, y + size, 0.0f }
        };

        float minX = 999999.0f;
        float maxX = -999999.0f;
        float minY = 999999.0f;
        float maxY = -999999.0f;

        for (int i = 0; i < 4; ++i)
        {
            float px = corners[i].x * m_lightViewProj.m[0][0] + corners[i].y * m_lightViewProj.m[1][0] + corners[i].z * m_lightViewProj.m[2][0] + m_lightViewProj.m[3][0];
            float py = corners[i].x * m_lightViewProj.m[0][1] + corners[i].y * m_lightViewProj.m[1][1] + corners[i].z * m_lightViewProj.m[2][1] + m_lightViewProj.m[3][1];
            float pw = corners[i].x * m_lightViewProj.m[0][3] + corners[i].y * m_lightViewProj.m[1][3] + corners[i].z * m_lightViewProj.m[2][3] + m_lightViewProj.m[3][3];

            if (pw != 0.0f)
            {
                px /= pw;
                py /= pw;
            }

            if (px < minX) minX = px;
            if (maxX > px) maxX = px;
            if (py < minY) minY = py;
            if (maxY > py) maxY = py;
        }

        if (maxX < -1.0f || minX > 1.0f)
            return false;
        
        if (maxY < -1.0f || minY > 1.0f)
            return false;

        return true;
    }
}
