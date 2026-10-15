#include "UIPassDispatcher.h"
#include "../StateManager.h"

namespace EterLib::Render {

void UIPassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) {
        EterBase::ModernLogger::Error("UIPassDispatcher::BeginPass - null device");
        return;
    }

    auto& sm = CStateManager::Instance();

    // 2D GUI Rendering requires Z-Buffer to be disabled
    sm.SaveRenderState(D3DRS_ZENABLE, FALSE);
    sm.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
    
    // UI is typically drawn with pre-transformed vertices or orthogonally, no culling needed
    sm.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // UI elements don't need 3D lighting calculations
    sm.SaveRenderState(D3DRS_LIGHTING, FALSE);
}

void UIPassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) {
        EterBase::ModernLogger::Error("UIPassDispatcher::EndPass - null device");
        return;
    }

    auto& sm = CStateManager::Instance();

    // Restore in reverse order of setting
    sm.RestoreRenderState(D3DRS_LIGHTING);
    sm.RestoreRenderState(D3DRS_CULLMODE);
    sm.RestoreRenderState(D3DRS_ZWRITEENABLE);
    sm.RestoreRenderState(D3DRS_ZENABLE);
}

} // namespace EterLib::Render

