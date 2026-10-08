#include "../../StdAfx.h"
#include "RenderTargetManager.h"
#include <stdexcept>

namespace EterLib::Render {

RenderTargetManager::~RenderTargetManager() {
    for (auto& item : m_pool) {
        if (item.texture) {
            item.texture->Release(); // Release the pool's internal reference
        }
    }
    m_pool.clear();
}

RenderTargetManager::State::~State() {
    if (renderTarget) {
        renderTarget->Release();
        renderTarget = nullptr;
    }
}

RenderTargetManager::State::State(State&& other) noexcept : renderTarget(other.renderTarget), viewport(other.viewport) {
    other.renderTarget = nullptr;
}

RenderTargetManager::State& RenderTargetManager::State::operator=(State&& other) noexcept {
    if (this != &other) {
        if (renderTarget) {
            renderTarget->Release();
        }
        renderTarget = other.renderTarget;
        viewport = other.viewport;
        other.renderTarget = nullptr;
    }
    return *this;
}

LPDIRECT3DTEXTURE9 RenderTargetManager::CreateRenderTarget(LPDIRECT3DDEVICE9 dev, uint32_t w, uint32_t h, D3DFORMAT fmt) {
    if (!dev) return nullptr;

    // Search the pool for an unused texture of the requested dimensions and format
    for (auto& pt : m_pool) {
        if (pt.width == w && pt.height == h && pt.format == fmt) {
            // Check COM reference count to determine if it is currently exclusively owned by the pool.
            pt.texture->AddRef();
            ULONG refCount = pt.texture->Release();
            if (refCount == 1) {
                // The pool is the only owner, so it is available for reuse!
                pt.texture->AddRef(); // Give a reference to the caller
                return pt.texture;
            }
        }
    }

    // No available pooled texture found, create a new one
    LPDIRECT3DTEXTURE9 texture = nullptr;
    HRESULT hr = dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, &texture, nullptr);
    if (FAILED(hr)) {
        return nullptr;
    }
    
    // texture comes with ref count 1 (owned by the pool)
    PooledTexture pt;
    pt.texture = texture;
    pt.width = w;
    pt.height = h;
    pt.format = fmt;
    m_pool.push_back(pt);
    
    // AddRef for the caller, so the texture now has ref count 2.
    // When the caller calls Release(), the ref count will drop to 1, returning it to the pool.
    texture->AddRef();
    return texture;
}

void RenderTargetManager::PushRenderTarget(LPDIRECT3DDEVICE9 dev, LPDIRECT3DSURFACE9 target) {
    if (!dev || !target) return;

    State state;
    dev->GetRenderTarget(0, &state.renderTarget); // Increases ref count of surface
    dev->GetViewport(&state.viewport);
    
    m_stateStack.push(std::move(state));

    dev->SetRenderTarget(0, target);
    
    D3DSURFACE_DESC desc;
    target->GetDesc(&desc);
    
    D3DVIEWPORT9 newVp;
    newVp.X = 0;
    newVp.Y = 0;
    newVp.Width = desc.Width;
    newVp.Height = desc.Height;
    newVp.MinZ = 0.0f;
    newVp.MaxZ = 1.0f;
    
    dev->SetViewport(&newVp);
}

void RenderTargetManager::PopRenderTarget(LPDIRECT3DDEVICE9 dev) {
    if (!dev || m_stateStack.empty()) return;

    const auto& state = m_stateStack.top();
    
    dev->SetRenderTarget(0, state.renderTarget);
    dev->SetViewport(&state.viewport);
    
    m_stateStack.pop(); 
}

} // namespace EterLib::Render

