#pragma once

#include "../StdAfx.h"
#include "../StateManager.h"

namespace EterLib::Render
{
    /**
     * @brief A RAII-based scope guard for managing D3DRS_COLORWRITEENABLE.
     *
     * This scope allows for selective enabling or disabling of color channel writes
     * during rendering operations. It is particularly useful for techniques like
     * Z-prepass (where all color writes are disabled) or alpha-only passes.
     *
     * The scope automatically saves the current render state upon construction
     * and restores it upon destruction via the central StateManager.
     */
    class ColorWriteScope
    {
    public:
        /**
         * @brief Constructs a new Color Write Scope.
         *
         * @param colorWriteEnableFlags The flags representing which color channels to write to.
         *                              Common values include:
         *                              0 - Disable all color writes (e.g., Z-prepass).
         *                              D3DCOLORWRITEENABLE_ALPHA - Write only to the alpha channel.
         *                              D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
         *                              D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA - Full color write.
         */
        explicit ColorWriteScope(DWORD colorWriteEnableFlags)
        {
            STATEMANAGER.SaveRenderState(D3DRS_COLORWRITEENABLE, colorWriteEnableFlags);
        }

        ~ColorWriteScope()
        {
            STATEMANAGER.RestoreRenderState(D3DRS_COLORWRITEENABLE);
        }

        // Prevent copying and assignment to ensure strict RAII semantics.
        ColorWriteScope(const ColorWriteScope&) = delete;
        ColorWriteScope& operator=(const ColorWriteScope&) = delete;
        ColorWriteScope(ColorWriteScope&&) = delete;
        ColorWriteScope& operator=(ColorWriteScope&&) = delete;
    };
} // namespace EterLib::Render

using ColorWriteScope = EterLib::Render::ColorWriteScope;
