#pragma once

#include <cstddef>
#include <array>
#include <utility>

// Required for D3D9 types
#ifndef IS_TESTING
#include "../StdAfx.h"
#else
#include "d3d9.h"
#include "d3dx9.h"
#endif

namespace EterLib::Render
{
    /**
     * @brief A utility class to capture and apply rendering states without memory allocations.
     * 
     * Specifically designed for capturing states before switching from 3D world rendering
     * to 2D GUI rendering and then restoring them afterward. Adheres to zero-allocation rules
     * by using std::array for internal buffers.
     */
    class RenderStateSnapshot
    {
    public:
        RenderStateSnapshot() = default;
        ~RenderStateSnapshot() = default;

        // Disallow copying/moving to prevent accidental large copies or state issues
        RenderStateSnapshot(const RenderStateSnapshot&) = delete;
        RenderStateSnapshot& operator=(const RenderStateSnapshot&) = delete;
        RenderStateSnapshot(RenderStateSnapshot&&) = delete;
        RenderStateSnapshot& operator=(RenderStateSnapshot&&) = delete;

        /**
         * @brief Captures the current relevant D3D9 pipeline states from CStateManager.
         */
        void Capture();

        /**
         * @brief Restores the captured D3D9 pipeline states to CStateManager.
         */
        void Apply() const;

    private:
        static constexpr std::size_t RENDER_STATE_COUNT = 8;
        static constexpr std::size_t TEXTURE_STAGE_COUNT = 1; // Only stage 0 usually needed for basic GUI
        static constexpr std::size_t TEXTURE_STAGE_STATE_COUNT = 2; 
        static constexpr std::size_t SAMPLER_STATE_COUNT = 3;
        static constexpr std::size_t TRANSFORM_COUNT = 3;

        // Captured Render States (ZENABLE, ALPHABLENDENABLE, CULLMODE, LIGHTING, etc.)
        std::array<std::pair<D3DRENDERSTATETYPE, DWORD>, RENDER_STATE_COUNT> m_renderStates{};

        // Captured Texture Stage States (e.g. COLOROP, ALPHAOP)
        std::array<std::pair<D3DTEXTURESTAGESTATETYPE, DWORD>, TEXTURE_STAGE_STATE_COUNT> m_textureStageStates{};
        
        // Captured Sampler States (e.g. MINFILTER, MAGFILTER, MIPFILTER)
        std::array<std::pair<D3DSAMPLERSTATETYPE, DWORD>, SAMPLER_STATE_COUNT> m_samplerStates{};

        // Captured Textures
        std::array<LPDIRECT3DBASETEXTURE9, TEXTURE_STAGE_COUNT> m_textures{};

        // Captured Transforms (WORLD, VIEW, PROJECTION)
        std::array<std::pair<D3DTRANSFORMSTATETYPE, D3DXMATRIX>, TRANSFORM_COUNT> m_transforms{};

        bool m_isCaptured = false;
    };
}
