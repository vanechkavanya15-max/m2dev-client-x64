#ifndef TEST_MODE_DISABLE_STDAFX
#include "../../StdAfx.h"
#endif

#pragma once

#include <vector>
#include <span>

namespace EterLib::Render
{
    class ActorBoundingFrustum
    {
    public:
        ActorBoundingFrustum() = default;
        ~ActorBoundingFrustum() = default;

        [[nodiscard]] bool IsActorVisible(const D3DVECTOR& pos, float radius, std::span<const D3DPLANE, 6> frustum) const noexcept;
        [[nodiscard]] bool IsBoxVisible(const D3DVECTOR& minPoint, const D3DVECTOR& maxPoint, std::span<const D3DPLANE, 6> frustum) const noexcept;
    };
}

