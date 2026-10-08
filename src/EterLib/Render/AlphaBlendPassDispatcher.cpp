#include "../StdAfx.h"
#include "AlphaBlendPassDispatcher.h"
#include <algorithm>
#include <utility>

namespace EterLib::Render {

void AlphaBlendPassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) return;

    // Save previous states
    dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_savedAlphaBlendEnable);
    dev->GetRenderState(D3DRS_SRCBLEND, &m_savedSrcBlend);
    dev->GetRenderState(D3DRS_DESTBLEND, &m_savedDestBlend);
    dev->GetRenderState(D3DRS_ZWRITEENABLE, &m_savedZWriteEnable);

    // Set new states for alpha blending pass
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
}

void AlphaBlendPassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) return;

    // Restore previous states
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, m_savedAlphaBlendEnable);
    dev->SetRenderState(D3DRS_SRCBLEND, m_savedSrcBlend);
    dev->SetRenderState(D3DRS_DESTBLEND, m_savedDestBlend);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, m_savedZWriteEnable);
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

