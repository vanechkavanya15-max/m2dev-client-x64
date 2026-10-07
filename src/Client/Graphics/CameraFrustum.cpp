#include "../../StdAfx.h"
#include "CameraFrustum.h"

namespace Client::Graphics
{
    void CameraFrustum::Update(const D3DXMATRIX& viewProjMatrix)
    {
        // Calculate the frustum planes from the view-projection matrix
        // The matrix is row-major (D3DXMATRIX is typically row-major)
        const D3DXMATRIX& mat = viewProjMatrix;
        
        // Left plane
        m_planes[0].a = mat._14 + mat._11;
        m_planes[0].b = mat._24 + mat._21;
        m_planes[0].c = mat._34 + mat._31;
        m_planes[0].d = mat._44 + mat._41;
        D3DXPlaneNormalize(&m_planes[0], &m_planes[0]);

        // Right plane
        m_planes[1].a = mat._14 - mat._11;
        m_planes[1].b = mat._24 - mat._21;
        m_planes[1].c = mat._34 - mat._31;
        m_planes[1].d = mat._44 - mat._41;
        D3DXPlaneNormalize(&m_planes[1], &m_planes[1]);

        // Top plane
        m_planes[2].a = mat._14 - mat._12;
        m_planes[2].b = mat._24 - mat._22;
        m_planes[2].c = mat._34 - mat._32;
        m_planes[2].d = mat._44 - mat._42;
        D3DXPlaneNormalize(&m_planes[2], &m_planes[2]);

        // Bottom plane
        m_planes[3].a = mat._14 + mat._12;
        m_planes[3].b = mat._24 + mat._22;
        m_planes[3].c = mat._34 + mat._32;
        m_planes[3].d = mat._44 + mat._42;
        D3DXPlaneNormalize(&m_planes[3], &m_planes[3]);

        // Near plane
        m_planes[4].a = mat._13;
        m_planes[4].b = mat._23;
        m_planes[4].c = mat._33;
        m_planes[4].d = mat._43;
        D3DXPlaneNormalize(&m_planes[4], &m_planes[4]);

        // Far plane
        m_planes[5].a = mat._14 - mat._13;
        m_planes[5].b = mat._24 - mat._23;
        m_planes[5].c = mat._34 - mat._33;
        m_planes[5].d = mat._44 - mat._43;
        D3DXPlaneNormalize(&m_planes[5], &m_planes[5]);
    }

    bool CameraFrustum::IsPointVisible(const D3DXVECTOR3& point, float margin) const
    {
        for (const auto& plane : m_planes)
        {
            if (D3DXPlaneDotCoord(&plane, &point) + margin < 0.0f)
            {
                return false;
            }
        }
        return true;
    }

    bool CameraFrustum::IsSphereVisible(const D3DXVECTOR3& center, float radius, float margin) const
    {
        for (const auto& plane : m_planes)
        {
            if (D3DXPlaneDotCoord(&plane, &center) + radius + margin < 0.0f)
            {
                return false;
            }
        }
        return true;
    }

    bool CameraFrustum::IsAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max, float margin) const
    {
        for (const auto& plane : m_planes)
        {
            // Find the point on the AABB that is furthest in the negative direction of the plane normal
            D3DXVECTOR3 p;
            p.x = (plane.a >= 0.0f) ? max.x : min.x;
            p.y = (plane.b >= 0.0f) ? max.y : min.y;
            p.z = (plane.c >= 0.0f) ? max.z : min.z;

            // If this point is on the negative side of the plane, the AABB is outside the frustum
            if (D3DXPlaneDotCoord(&plane, &p) + margin < 0.0f)
            {
                return false;
            }
        }
        return true;
    }
}
