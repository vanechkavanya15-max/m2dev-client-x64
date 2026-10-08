#include "../StdAfx.h"
#include "AlphaBlendPassDispatcher.h"
#include <algorithm>
#include <utility>

#include "../StateManager.h"

namespace EterLib::Render {

void AlphaBlendPassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) return;

    STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
}

void AlphaBlendPassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) return;

    STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
    STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);
    STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
    STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
}

void AlphaBlendPassDispatcher::Dispatch(float depth, std::function<void()> renderCommand) {
    if (!renderCommand) return;
    m_items.push_back({depth, std::move(renderCommand)});
}

void AlphaBlendPassDispatcher::Flush() {
    if (m_items.empty()) return;

    // Sort from furthest to nearest (descending depth)
    std::sort(m_items.begin(), m_items.end(), [](const AlphaBlendItem& a, const AlphaBlendItem& b) {
        return a.depth > b.depth;
    });

    for (const auto& item : m_items) {
        if (item.renderCommand) {
            item.renderCommand();
        }
    }

    m_items.clear();
}

} // namespace EterLib::Render

