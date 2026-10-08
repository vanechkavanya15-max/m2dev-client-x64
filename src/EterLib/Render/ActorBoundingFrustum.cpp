#include "ActorBoundingFrustum.h"

namespace EterLib::Render
{
    bool ActorBoundingFrustum::IsActorVisible(const D3DVECTOR& pos, float radius, std::span<const D3DPLANE, 6> frustum) const noexcept
    {
        for (const auto& plane : frustum)
        {
            // Early rejection for actors (e.g. standing behind camera)
            // A plane equation ax + by + cz + d = 0 gives the distance to a point
            float distance = plane.a * pos.x + plane.b * pos.y + plane.c * pos.z + plane.d;
            if (distance < -radius)
            {
                return false;
            }
        }
        return true;
    }

    bool ActorBoundingFrustum::IsBoxVisible(const D3DVECTOR& minPoint, const D3DVECTOR& maxPoint, std::span<const D3DPLANE, 6> frustum) const noexcept
    {
        for (const auto& plane : frustum)
        {
            // Calculate the minimum distance vertex of the box to the plane
            D3DVECTOR p;
            p.x = (plane.a >= 0.0f) ? maxPoint.x : minPoint.x;
            p.y = (plane.b >= 0.0f) ? maxPoint.y : minPoint.y;
            p.z = (plane.c >= 0.0f) ? maxPoint.z : minPoint.z;

            // If the maximum projection is outside, the box is outside
            float distance = plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d;
            if (distance < 0.0f)
            {
                return false;
            }
        }
        return true;
    }
}

