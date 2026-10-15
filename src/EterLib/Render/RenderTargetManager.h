#pragma once

#include <d3d9.h>
#include <cstdint>
#include <stack>
#include <vector>

namespace EterLib::Render {

class RenderTargetManager {
public:
    RenderTargetManager() = default;
    ~RenderTargetManager();

    RenderTargetManager(const RenderTargetManager&) = delete;
    RenderTargetManager& operator=(const RenderTargetManager&) = delete;

    LPDIRECT3DTEXTURE9 CreateRenderTarget(LPDIRECT3DDEVICE9 dev, uint32_t w, uint32_t h, D3DFORMAT fmt);
    void PushRenderTarget(LPDIRECT3DDEVICE9 dev, LPDIRECT3DSURFACE9 target);
    void PopRenderTarget(LPDIRECT3DDEVICE9 dev);

private:
    struct State {
        LPDIRECT3DSURFACE9 renderTarget = nullptr;
        D3DVIEWPORT9 viewport = {};

        State() = default;
        ~State();
        
        State(const State&) = delete;
        State& operator=(const State&) = delete;
        State(State&& other) noexcept;
        State& operator=(State&& other) noexcept;
    };
    
    std::stack<State> m_stateStack;

    struct PooledTexture {
        LPDIRECT3DTEXTURE9 texture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        D3DFORMAT format = D3DFMT_UNKNOWN;
    };

    std::vector<PooledTexture> m_pool;
};

} // namespace EterLib::Render

