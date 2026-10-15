#include "OpaquePassDispatcher.h"
#include "../StateManager.h"

namespace EterLib::Render
{
    void OpaquePassDispatcher::BeginPass(LPDIRECT3DDEVICE9 /*dev*/) noexcept
    {
        STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        STATEMANAGER.SaveRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    }

    void OpaquePassDispatcher::EndPass(LPDIRECT3DDEVICE9 /*dev*/) noexcept
    {
        STATEMANAGER.RestoreRenderState(D3DRS_COLORWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZFUNC);
        STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
    }
}

