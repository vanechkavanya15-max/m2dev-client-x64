#pragma once

#include <d3d9.h>
#include "RenderStateTypes.h"
#include "../StateManager.h"

namespace EterLib::Render {

/**
 * @brief An RAII scope guard for D3D blend states.
 * 
 * Automatically saves the current blend states upon construction, sets new states,
 * and restores the previous states upon destruction.
 */
class BlendStateScope {
public:
    /**
     * @brief Constructs a scope guard with a specific predefined blend mode.
     * @param mode The blend mode preset to apply.
     */
    explicit BlendStateScope(BlendMode mode) {
        ApplyMode(mode);
    }

    /**
     * @brief Constructs a scope guard with custom blend parameters.
     * @param blendEnable Whether to enable alpha blending.
     * @param srcBlend The source blend factor.
     * @param destBlend The destination blend factor.
     * @param blendOp The blend operation (default is ADD).
     */
    BlendStateScope(bool blendEnable, D3DBLEND srcBlend, D3DBLEND destBlend, D3DBLENDOP blendOp = D3DBLENDOP_ADD) {
        auto& sm = CStateManager::Instance();
        sm.SaveRenderState(D3DRS_ALPHABLENDENABLE, blendEnable ? TRUE : FALSE);
        sm.SaveRenderState(D3DRS_SRCBLEND, srcBlend);
        sm.SaveRenderState(D3DRS_DESTBLEND, destBlend);
        sm.SaveRenderState(D3DRS_BLENDOP, blendOp);
    }

    /**
     * @brief Destructor. Restores the previously saved blend states.
     */
    ~BlendStateScope() {
        auto& sm = CStateManager::Instance();
        sm.RestoreRenderState(D3DRS_BLENDOP);
        sm.RestoreRenderState(D3DRS_DESTBLEND);
        sm.RestoreRenderState(D3DRS_SRCBLEND);
        sm.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
    }

    // Prevent copying and assignment
    BlendStateScope(const BlendStateScope&) = delete;
    BlendStateScope& operator=(const BlendStateScope&) = delete;

private:
    void ApplyMode(BlendMode mode) {
        auto& sm = CStateManager::Instance();
        
        switch (mode) {
            case BlendMode::Opaque:
                sm.SaveRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                sm.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
                sm.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
                sm.SaveRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                break;
                
            case BlendMode::AlphaBlend:
                sm.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                sm.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                sm.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                sm.SaveRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                break;
                
            case BlendMode::Additive:
                sm.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                sm.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                sm.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
                sm.SaveRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                break;
                
            case BlendMode::Multiply:
                sm.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                sm.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
                sm.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
                sm.SaveRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                break;
        }
    }
};

} // namespace EterLib::Render
