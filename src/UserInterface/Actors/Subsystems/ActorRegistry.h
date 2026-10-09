#pragma once

#include <cstdint>
#include <unordered_map>
#include <functional>

#include "UserInterface/Contracts/IGameEvents.h"

// Deklaracja wyprzedzajaca instancji aktora
class CInstanceBase;

#ifndef DWORD
typedef unsigned long DWORD;
#endif

namespace UserInterface::Actors::Subsystems
{
    /**
     * @brief Czysty, jednowatkowy rejestr instancji postaci (CInstanceBase).
     * 
     * Odpowiada za przechowywanie oraz bezpieczne zarzadzanie cyklem zycia
     * referencji do instancji postaci w pamieci klienta.
     * 
     * Guardraile bezpieczenstwa:
     * 1. DANGLING POINTER SAFETY: Wskaznik jest natychmiast uniewazniany
     *    w mapie przed wyrejestrowaniem lub rozgloszeniem zdarzenia,
     *    co uniemozliwia odczytanie wiszacego wskaznika.
     * 2. SINGLE THREADED: Pelna deterministycznosc bez zadnych std::mutex
     *    ani operacji blokujacych w glownym watku gry.
     * 3. Powiadamianie IGameEventSink::OnActorDead w momencie wyrejestrowania.
     */
    class ActorRegistry
    {
    public:
        using ActorMap = std::unordered_map<DWORD, CInstanceBase*>;
        using DeadCallback = std::function<void(DWORD)>;

        ActorRegistry() = default;
        explicit ActorRegistry(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;
        ~ActorRegistry();

        // Rejestr jest unikalnym zasobem menedzera - brak kopiowania
        ActorRegistry(const ActorRegistry&) = delete;
        ActorRegistry& operator=(const ActorRegistry&) = delete;
        ActorRegistry(ActorRegistry&&) noexcept = default;
        ActorRegistry& operator=(ActorRegistry&&) noexcept = default;

        /**
         * @brief Rejestruje instancje aktora pod wskazanym unikalnym VID.
         * @param dwVID Identyfikator VirtualID postaci (musi byc > 0).
         * @param pInst Wskaznik na poprawna instancje CInstanceBase.
         * @return true jesli rejestracja powiodla sie, false przy blednych danych wejsciowych.
         */
        bool RegisterActor(DWORD dwVID, CInstanceBase* pInst);

        /**
         * @brief Wyrejestrowuje aktora, uniewaznia wskaznik i powiadamia odbiornik zdarzen.
         * @param dwVID Identyfikator VirtualID postaci.
         * @return true jesli aktor zostal znaleziony i usuniety, false jesli nie istnial.
         */
        bool UnregisterActor(DWORD dwVID);

        /**
         * @brief Bezpieczne wyszukiwanie wskaznika na aktora po VID.
         * @param dwVID Identyfikator VirtualID postaci.
         * @return Wskaznik na CInstanceBase lub nullptr jesli brak lub uniewazniony.
         */
        [[nodiscard]] CInstanceBase* FindActor(DWORD dwVID) const noexcept;

        /**
         * @brief Czysci wszystkie zarejestrowane instancje zerujac wskazniki.
         */
        void ClearAll() noexcept;

        /**
         * @brief Sprawdza czy dany aktor jest zarejestrowany i poprawny.
         */
        [[nodiscard]] bool ContainsActor(DWORD dwVID) const noexcept;

        /**
         * @brief Zwraca biezaca liczbe zarejestrowanych aktorow.
         */
        [[nodiscard]] size_t GetActorCount() const noexcept;

        /**
         * @brief Informuje czy rejestr jest pusty.
         */
        [[nodiscard]] bool IsEmpty() const noexcept;

        /**
         * @brief Konfiguruje zewnetrzny odbiornik zdarzen IGameEventSink.
         */
        void SetEventSink(UserInterface::Contracts::IGameEventSink* pEventSink) noexcept;

        /**
         * @brief Zwraca przypisany odbiornik zdarzen IGameEventSink.
         */
        [[nodiscard]] UserInterface::Contracts::IGameEventSink* GetEventSink() const noexcept;

        /**
         * @brief Rejestruje opcjonalny callback powiadamiajacy o smierci/usunieciu aktora.
         */
        void SetDeadCallback(DeadCallback callback) noexcept;

        /**
         * @brief Udostepnia staly wglad w wewnetrzna mape instancji aktorow.
         */
        [[nodiscard]] const ActorMap& GetAllActors() const noexcept;

        /**
         * @brief Wykonuje funkcje dla kazdego aktywnego aktora w rejestrze.
         */
        template <typename Func>
        void ForEachActor(Func&& func) const
        {
            for (const auto& [vid, pActor] : m_actors)
            {
                if (pActor != nullptr)
                {
                    func(vid, pActor);
                }
            }
        }

    private:
        ActorMap m_actors;
        UserInterface::Contracts::IGameEventSink* m_pEventSink{nullptr};
        DeadCallback m_deadCallback{nullptr};
    };
}

namespace UserInterface::Actors
{
    using Subsystems::ActorRegistry;
}
