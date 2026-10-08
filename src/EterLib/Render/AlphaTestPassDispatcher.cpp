#include "../StdAfx.h"
#include "AlphaTestPassDispatcher.h"

#include "../StateManager.h"

namespace EterLib::Render
{
    void AlphaTestPassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
            return;

        STATEMANAGER.SaveRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_ALPHAREF, 128);
        STATEMANAGER.SaveRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
        STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
    }

    void AlphaTestPassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
            return;

        STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHAFUNC);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHAREF);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHATESTENABLE);
    }
}

