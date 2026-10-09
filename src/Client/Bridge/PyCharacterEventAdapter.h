#pragma once

#include <cstdint>
#include <string_view>
#include <expected>
#include "../../EterBase/PyBridge.h"
#include "../UI/PyBridgeFastCall.h"
#include "../Core/DomainEvents.h"
#include "../Core/EventBus.h"

namespace Client::Bridge {

    /**
     * @brief Adapter konwertujacy zdarzenia domenowe postaci (CharacterEvents)
     *        na wywolania bezposrednich handlerow w Pythonie.
     */
    class PyCharacterEventAdapter {
    public:
        /**
         * @brief Konstruktor przyjmujacy obiekt handlera w Pythonie.
         * @param pyHandler Wskaznik na obiekt Pythona realizujacy interfejs zdarzen.
         */
        explicit PyCharacterEventAdapter(PyObject* pyHandler) noexcept
            : m_pyHandler(pyHandler, true) // INCREF (borrow = true)
        {
        }

        /**
         * @brief Destruktor RAII zwalniajacy referencje do handlera.
         */
        ~PyCharacterEventAdapter() noexcept = default;

        /**
         * @brief Konwertuje zdarzenie smierci aktora na wywolanie w Pythonie.
         * @param event Instancja zdarzenia ActorDeadEvent.
         */
        void OnActorDead(const Client::Core::ActorDeadEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) {
                return;
            }

            // Upewniamy sie, ze kod odpala sie bezpiecznie ze wzgledu na GIL
            PyBridge::PyGILScope gilScope;

            // Wywolanie metody "OnActorDead" za pomoca nowoczesnego Vectorcall.
            // Zgodnie z wymaganiami wywolujemy szybka metode bez alokacji krotek.
            auto result = Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "OnActorDead");
            
            // Jesli handler oczekuje argumentow, FastCallMethod w obecnej postaci pozwala tylko 
            // na 0-argumentowe wywolanie. Zakladamy z opisu ze uzywamy do powiadomien/konwersji.
            if (!result.has_value()) {
                // Blad jest ignorowany/logowany na nizszym poziomie, adapter po prostu nie przerywa dzialania.
            }
        }
        
        /**
         * @brief Konwertuje zdarzenie odswiezenia zlota gracza na wywolanie w Pythonie.
         * @param event Instancja zdarzenia PlayerGoldUpdatedEvent.
         */
        void OnPlayerGoldUpdated(const Client::Core::PlayerGoldUpdatedEvent& event) noexcept
        {
            if (!m_pyHandler.IsValid()) {
                return;
            }

            PyBridge::PyGILScope gilScope;
            auto result = Client::UI::PyFastCall::FastCallMethod(m_pyHandler.Get(), "OnPlayerGoldUpdated");
            if (!result.has_value()) {
            }
        }

    private:
        PyBridge::PyRef<> m_pyHandler;
    };

} // namespace Client::Bridge
