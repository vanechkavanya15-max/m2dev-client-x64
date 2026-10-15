#pragma once

#include <cstdint>
#include "../StateManager.h"

namespace EterLib::Render
{
    /**
     * @brief A zero-conflict RAII scope guard for managing DirectX 9 alpha test rendering states.
     * 
     * Handles D3DRS_ALPHATESTENABLE, D3DRS_ALPHAREF, and D3DRS_ALPHAFUNC.
     * Automatically captures the current states upon initialization and strictly restores them
     * when going out of scope.
     */
    class AlphaTestScope
    {
    public:
        /**
         * @brief Creates an AlphaTestScope optimized for tree leaves rendering.
         * 
         * Applies the following presets:
         * - AlphaTestEnable = TRUE
         * - AlphaRef = 128
         * - AlphaFunc = D3DCMP_GREATEREQUAL
         */
        [[nodiscard]] static AlphaTestScope CreateForTreeLeaves()
        {
            return AlphaTestScope(true, 128, D3DCMP_GREATEREQUAL);
        }

        /**
         * @brief Constructs the AlphaTestScope and applies new states.
         * 
         * @param enable Whether alpha testing should be enabled.
         * @param alphaRef The reference alpha value (0-255).
         * @param alphaFunc The comparison function (D3DCMPFUNC).
         */
        explicit AlphaTestScope(bool enable, DWORD alphaRef, DWORD alphaFunc)
            : m_savedEnable(0),
              m_savedAlphaRef(0),
              m_savedAlphaFunc(0),
              m_active(true)
        {
            // Retrieve current states
            STATEMANAGER.GetRenderState(D3DRS_ALPHATESTENABLE, &m_savedEnable);
            STATEMANAGER.GetRenderState(D3DRS_ALPHAREF, &m_savedAlphaRef);
            STATEMANAGER.GetRenderState(D3DRS_ALPHAFUNC, &m_savedAlphaFunc);

            // Apply new states
            STATEMANAGER.SetRenderState(D3DRS_ALPHATESTENABLE, enable ? TRUE : FALSE);
            STATEMANAGER.SetRenderState(D3DRS_ALPHAREF, alphaRef);
            STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);
        }

        /**
         * @brief Destructs the scope and restores previous states.
         */
        ~AlphaTestScope()
        {
            if (m_active)
            {
                STATEMANAGER.SetRenderState(D3DRS_ALPHATESTENABLE, m_savedEnable);
                STATEMANAGER.SetRenderState(D3DRS_ALPHAREF, m_savedAlphaRef);
                STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, m_savedAlphaFunc);
            }
        }

        // Prevent copying to enforce strict RAII bounds
        AlphaTestScope(const AlphaTestScope&) = delete;
        AlphaTestScope& operator=(const AlphaTestScope&) = delete;

        // Move semantics
        AlphaTestScope(AlphaTestScope&& other) noexcept
            : m_savedEnable(other.m_savedEnable),
              m_savedAlphaRef(other.m_savedAlphaRef),
              m_savedAlphaFunc(other.m_savedAlphaFunc),
              m_active(other.m_active)
        {
            other.m_active = false;
        }

        AlphaTestScope& operator=(AlphaTestScope&& other) noexcept
            {
                if (this != &other)
                {
                    if (m_active)
                    {
                        STATEMANAGER.SetRenderState(D3DRS_ALPHATESTENABLE, m_savedEnable);
                        STATEMANAGER.SetRenderState(D3DRS_ALPHAREF, m_savedAlphaRef);
                        STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, m_savedAlphaFunc);
                    }

                    m_savedEnable = other.m_savedEnable;
                    m_savedAlphaRef = other.m_savedAlphaRef;
                    m_savedAlphaFunc = other.m_savedAlphaFunc;
                    m_active = other.m_active;

                    other.m_active = false;
                }
                return *this;
            }

    private:
        DWORD m_savedEnable;
        DWORD m_savedAlphaRef;
        DWORD m_savedAlphaFunc;
        bool m_active;
    };
}
