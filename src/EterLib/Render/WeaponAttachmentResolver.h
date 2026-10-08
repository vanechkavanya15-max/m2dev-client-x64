#pragma once

#include <d3d9.h>
#include <d3dx9.h>

namespace EterLib::Render
{
    class WeaponAttachmentResolver
    {
    public:
        [[nodiscard]] D3DMATRIX ResolveAttachment(const D3DMATRIX& boneTransform, const D3DMATRIX& actorWorld, const D3DMATRIX& localOffset) const noexcept;
    };
}

