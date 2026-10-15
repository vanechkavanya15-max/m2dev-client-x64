#pragma once

#include "EterLib/StdAfx.h"

namespace EterLib::Render
{
    class AlphaTestPassDispatcher
    {
    public:
        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;
    };
}

