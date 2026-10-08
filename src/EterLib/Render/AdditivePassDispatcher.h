#pragma once

#include <d3d9.h>
#include "../StateManager.h"

namespace EterLib::Render
{
    /**
     * @brief Zero-conflict AdditivePassDispatcher for managing D3D9 additive blending passes.
     *
     * Used for rendering glares, skill effects, and auras.
     * Manages ALPHABLENDENABLE, SRCBLEND, DESTBLEND and ZWRITEENABLE.
     */
    class AdditivePassDispatcher
    {
    public:
        AdditivePassDispatcher() = default;
        ~AdditivePassDispatcher() = default;

        // Prevent copying and moving
        AdditivePassDispatcher(const AdditivePassDispatcher&) = delete;
        AdditivePassDispatcher& operator=(const AdditivePassDispatcher&) = delete;
        AdditivePassDispatcher(AdditivePassDispatcher&&) = delete;
        AdditivePassDispatcher& operator=(AdditivePassDispatcher&&) = delete;

        /**
         * @brief Begins the additive rendering pass by saving and modifying necessary states.
         * @param dev The active D3D9 device.
         */
        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;

        /**
         * @brief Ends the additive rendering pass by restoring previously saved states.
         * @param dev The active D3D9 device.
         */
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;
    };
}

