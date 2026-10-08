#pragma once

#include <d3d9.h>
#include <optional>
#include "../StateManager.h"
#include "ColorWriteScope.h"
#include "PixelShaderScope.h"

namespace EterLib::Render
{
    /**
     * @brief A RAII-based scope guard for managing D3DRS_ZWRITEENABLE.
     */
    class ZBufferScope
    {
    public:
        ZBufferScope()
        {
            STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
        }

        ~ZBufferScope()
        {
            STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        }

        ZBufferScope(const ZBufferScope&) = delete;
        ZBufferScope& operator=(const ZBufferScope&) = delete;
        ZBufferScope(ZBufferScope&&) = delete;
        ZBufferScope& operator=(ZBufferScope&&) = delete;
    };

    /**
     * @brief Manages D3D9 pipeline configuration for an early Z-pass (Depth Prepass).
     */
    class DepthPrepassDispatcher
    {
    public:
        DepthPrepassDispatcher() = default;
        ~DepthPrepassDispatcher() = default;

        /**
         * @brief Begins the depth prepass, configuring D3D9 states.
         * 
         * @param dev The active Direct3D device.
         */
        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;

        /**
         * @brief Ends the depth prepass, restoring prior D3D9 states.
         * 
         * @param dev The active Direct3D device.
         */
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;

    private:
        std::optional<ColorWriteScope> m_colorWriteScope;
        std::optional<ZBufferScope> m_zBufferScope;
        std::optional<PixelShaderScope> m_pixelShaderScope;
    };
} // namespace EterLib::Render

