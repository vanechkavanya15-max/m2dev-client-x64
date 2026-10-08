#include "StdAfx.h"
#include "AlphaTestPassDispatcher.h"

namespace EterLib::Render
{
    void AlphaTestPassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
            return;

        dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        dev->SetRenderState(D3DRS_ALPHAREF, 128);
        dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    }

    void AlphaTestPassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
            return;

        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    }
}

