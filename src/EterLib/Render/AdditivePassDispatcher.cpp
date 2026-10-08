#include "AdditivePassDispatcher.h"

namespace EterLib::Render
{
    void AdditivePassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // Save current states and set additive blending preset:
        // ALPHABLENDENABLE = TRUE
        // SRCBLEND = ONE
        // DESTBLEND = ONE
        // ZWRITEENABLE = FALSE
        
        STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }

    void AdditivePassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // Restore in reverse order to ensure LIFO consistency if StateManager relies on stack
        STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);
        STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
        STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
    }
}

