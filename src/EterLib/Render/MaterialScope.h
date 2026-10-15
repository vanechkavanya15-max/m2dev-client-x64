#pragma once

#ifndef __linux__
#include "../StdAfx.h"
#include "../StateManager.h"
#endif

namespace EterLib::Render
{
    /**
     * @class MaterialScope
     * @brief Zarzadza materialami D3D (D3DMATERIAL9) oraz D3DRS_DIFFUSEMATERIALSOURCE poprzez RAII.
     * Zgodnie z zasada zero-conflict oraz bez alokacji na stercie (heap).
     */
    class MaterialScope
    {
    public:
        /**
         * @brief Inicjuje nowy material i zrodlo diffuse, zapisujac poprzedni stan na stosie.
         */
        MaterialScope(const D3DMATERIAL9& newMaterial, DWORD diffuseSource = D3DMCS_MATERIAL)
        {
            // Pobieramy aktualny stan z menedzera stanu
            STATEMANAGER.GetMaterial(&m_oldMaterial);
            STATEMANAGER.GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, &m_oldDiffuseSource);

            // Aplikujemy nowe stany
            STATEMANAGER.SetMaterial(&newMaterial);
            STATEMANAGER.SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, diffuseSource);
        }

        /**
         * @brief Przywraca wczesniejszy material i zrodlo diffuse po wyjsciu ze scope'a.
         */
        ~MaterialScope()
        {
            STATEMANAGER.SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, m_oldDiffuseSource);
            STATEMANAGER.SetMaterial(&m_oldMaterial);
        }

        // Zablokowane kopiowanie i przenoszenie w celu zapewnienia semantyki RAII
        MaterialScope(const MaterialScope&) = delete;
        MaterialScope& operator=(const MaterialScope&) = delete;
        MaterialScope(MaterialScope&&) = delete;
        MaterialScope& operator=(MaterialScope&&) = delete;

    private:
        D3DMATERIAL9 m_oldMaterial;
        DWORD m_oldDiffuseSource;
    };
} // namespace EterLib::Render
