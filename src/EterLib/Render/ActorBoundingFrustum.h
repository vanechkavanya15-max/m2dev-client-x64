#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include <d3dx9.h>
#include <vector>
#include <span>

namespace EterLib::Render
{
    class ActorBoundingFrustum
    {
    public:
        ActorBoundingFrustum() = default;
        ~ActorBoundingFrustum() = default;

        [[nodiscard]] bool IsActorVisible(const D3DVECTOR& pos, float radius, std::span<const D3DXPLANE, 6> frustum) const noexcept;
        [[nodiscard]] bool IsBoxVisible(const D3DVECTOR& minPoint, const D3DVECTOR& maxPoint, std::span<const D3DXPLANE, 6> frustum) const noexcept;
    };
}
