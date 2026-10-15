#pragma once

#include <d3d9.h>

namespace EterLib::Render
{
    class OpaquePassDispatcher
    {
    public:
        OpaquePassDispatcher() noexcept = default;
        ~OpaquePassDispatcher() = default;

        OpaquePassDispatcher(const OpaquePassDispatcher&) = delete;
        OpaquePassDispatcher& operator=(const OpaquePassDispatcher&) = delete;

        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;
    };
}

