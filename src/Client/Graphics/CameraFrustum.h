#pragma once
#include "StdAfx.h"
#include <d3dx9.h>
#include <array>

namespace Client::Graphics
{
    class CameraFrustum
    {
    public:
        CameraFrustum() = default;
        ~CameraFrustum() = default;

        void Update(const D3DXMATRIX& viewProjMatrix);

        bool IsPointVisible(const D3DXVECTOR3& point, float margin = 0.0f) const;
        bool IsSphereVisible(const D3DXVECTOR3& center, float radius, float margin = 0.0f) const;
        bool IsAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max, float margin = 0.0f) const;

    private:
        std::array<D3DXPLANE, 6> m_planes;
    };
}
