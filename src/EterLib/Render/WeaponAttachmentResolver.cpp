#include "WeaponAttachmentResolver.h"
#include <cstring>

namespace EterLib::Render
{
    D3DMATRIX WeaponAttachmentResolver::ResolveAttachment(const D3DMATRIX& boneTransform, const D3DMATRIX& actorWorld, const D3DMATRIX& localOffset) const noexcept
    {
        D3DXMATRIX result;
        
        // Matrix multiplication order for DirectX: local * bone * actor
        D3DXMatrixMultiply(&result, reinterpret_cast<const D3DXMATRIX*>(&localOffset), reinterpret_cast<const D3DXMATRIX*>(&boneTransform));
        D3DXMatrixMultiply(&result, &result, reinterpret_cast<const D3DXMATRIX*>(&actorWorld));

        D3DMATRIX d3dResult;
        std::memcpy(&d3dResult, &result, sizeof(D3DMATRIX));
        return d3dResult;
    }
}

