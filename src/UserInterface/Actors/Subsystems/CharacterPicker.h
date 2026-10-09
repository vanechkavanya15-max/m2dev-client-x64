#pragma once

#include <cstdint>
#include <vector>
#include <d3dx9math.h>

#include "EterLib/Ray.h"

// Deklaracje wyprzedzajace
class CInstanceBase;

#ifndef DWORD
typedef unsigned long DWORD;
#endif

namespace UserInterface::Actors::Subsystems
{
    class ActorRegistry;

    /**
     * @brief Dedykowany podsystem selekcji postaci kursorem myszy (Picking).
     * 
     * Odpowiada za testy kolizji promienia kamery z bryla aktora,
     * wyliczanie odleglosci 3D oraz selekcje najblizszej postaci.
     * Calkowicie odciaza glowny menedzer postaci (CPythonCharacterManager).
     * 
     * Guardraile:
     * 1. DANGLING POINTER SAFETY: Brak pamietania uniewaznionych instancji.
     *    Metoda OnActorRemoved natychmiast zeruje wybrana instancje.
     * 2. SINGLE THREADED: Zadnych muteksow ani synchronizacji miedzywatkowej.
     */
    class CharacterPicker
    {
    public:
        CharacterPicker() = default;
        explicit CharacterPicker(const ActorRegistry* pRegistry) noexcept;
        ~CharacterPicker();

        CharacterPicker(const CharacterPicker&) = delete;
        CharacterPicker& operator=(const CharacterPicker&) = delete;
        CharacterPicker(CharacterPicker&&) noexcept = default;
        CharacterPicker& operator=(CharacterPicker&&) noexcept = default;

        /**
         * @brief Wykonuje test selekcji postaci na podstawie pozycji kursora i promienia kamery.
         * @param mouseX Pozycja X kursora myszy na ekranie.
         * @param mouseY Pozycja Y kursora myszy na ekranie.
         * @param cameraRay Promien rzutowany z kamery w przestrzen swiata 3D.
         * @return CInstanceBase* Wskaznik na wybrana instancje lub nullptr jesli nic nie wskazano.
         */
        CInstanceBase* Pick(long mouseX, long mouseY, const CRay& cameraRay);

        /**
         * @brief Wygodny wariant Pick pobierajacy aktywny promien z biezacej kamery.
         * @param mouseX Pozycja X kursora myszy na ekranie.
         * @param mouseY Pozycja Y kursora myszy na ekranie.
         * @return CInstanceBase* Wskaznik na wybrana instancje lub nullptr.
         */
        CInstanceBase* Pick(long mouseX, long mouseY);

        /**
         * @brief Zwraca biezaco wskazana instancje postaci.
         */
        [[nodiscard]] CInstanceBase* GetPickedActor() const noexcept;

        /**
         * @brief Zeruje wybor i powiadamia poprzednio zaznaczona postac (OnUnselected).
         */
        void ClearPick() noexcept;

        /**
         * @brief Przypisuje rejestr aktorow uzywany do przeszukiwania kandydatow.
         */
        void SetActorRegistry(const ActorRegistry* pRegistry) noexcept;

        /**
         * @brief Pobiera aktualnie podpiety rejestr aktorow.
         */
        [[nodiscard]] const ActorRegistry* GetActorRegistry() const noexcept;

        /**
         * @brief Przypisuje instancje glownego gracza (dla ustalenia priorytetow).
         */
        void SetMainActor(CInstanceBase* pMainActor) noexcept;

        /**
         * @brief Pobiera wskaznik na glowna postac.
         */
        [[nodiscard]] CInstanceBase* GetMainActor() const noexcept;

        /**
         * @brief Zwraca rzutowana pozycje 2D wybranej postaci na ekranie.
         */
        [[nodiscard]] const D3DXVECTOR2& GetPickedActorScreenPos() const noexcept;

        /**
         * @brief Zwraca odleglosc kwadratowa do wybranego aktora od oka kamery.
         */
        [[nodiscard]] float GetPickedDistance() const noexcept;

        /**
         * @brief Informuje czy jakas postac jest aktualnie wybrana.
         */
        [[nodiscard]] bool HasPickedActor() const noexcept;

        /**
         * @brief Reaguje na wyrejestrowanie aktora, gwarantujac brak dangling pointera.
         * @param dwVID VirtualID usuwanego aktora.
         */
        void OnActorRemoved(DWORD dwVID) noexcept;

    private:
        struct PickCandidate
        {
            CInstanceBase* pActor{nullptr};
            bool isDead{false};
            bool isMainActor{false};
            float distanceSq{0.0f};
            float rayIntersectionT{0.0f};
        };

        const ActorRegistry* m_pRegistry{nullptr};
        CInstanceBase* m_pMainActor{nullptr};
        CInstanceBase* m_pPickedActor{nullptr};

        D3DXVECTOR2 m_pickedScreenPos{0.0f, 0.0f};
        long m_lastMouseX{0};
        long m_lastMouseY{0};
        float m_pickedDistance{-1.0f};
    };
}

namespace UserInterface::Actors
{
    using Subsystems::CharacterPicker;
}
