#include "DepthPrepassDispatcher.h"

namespace EterLib::Render
{
    void DepthPrepassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // 1. Disable color writes (RGBA)
        m_colorWriteScope.emplace(0);

        // 2. Enable Z-buffer writing
        m_zBufferScope.emplace();

        // 3. Disable pixel shaders (null pixel shader for pure depth pass)
        m_pixelShaderScope.emplace(nullptr);
    }

    void DepthPrepassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // Restore scopes in reverse order of creation
        m_pixelShaderScope.reset();
        m_zBufferScope.reset();
        m_colorWriteScope.reset();
    }
} // namespace EterLib::Render

