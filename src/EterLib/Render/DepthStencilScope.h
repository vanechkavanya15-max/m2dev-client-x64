#pragma once

#include "RenderStateTypes.h"
#include "../../EterLib/StateManager.h"

namespace EterLib::Render
{
    class DepthStencilScope
    {
    public:
        explicit DepthStencilScope(DepthMode mode)
        {
            switch (mode)
            {
            case DepthMode::ReadWrite:
                STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, TRUE);
                STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
                STATEMANAGER.SaveRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
                STATEMANAGER.SaveRenderState(D3DRS_STENCILENABLE, FALSE);
                break;
            case DepthMode::ReadOnly:
                STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, TRUE);
                STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
                STATEMANAGER.SaveRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
                STATEMANAGER.SaveRenderState(D3DRS_STENCILENABLE, FALSE);
                break;
            case DepthMode::Disabled:
                STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, FALSE);
                STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
                STATEMANAGER.SaveRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
                STATEMANAGER.SaveRenderState(D3DRS_STENCILENABLE, FALSE);
                break;
            }
        }

        ~DepthStencilScope()
        {
            STATEMANAGER.RestoreRenderState(D3DRS_STENCILENABLE);
            STATEMANAGER.RestoreRenderState(D3DRS_ZFUNC);
            STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
            STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
        }

        // Non-copyable and non-movable
        DepthStencilScope(const DepthStencilScope&) = delete;
        DepthStencilScope& operator=(const DepthStencilScope&) = delete;
        DepthStencilScope(DepthStencilScope&&) = delete;
        DepthStencilScope& operator=(DepthStencilScope&&) = delete;
    };
}
