#pragma once

#include <optional>
#include <cstdint>

#include "EterBase/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/Events.h"

namespace Client::Gameplay
{
    /**
     * @brief Kontroler bezpiecznego zarzadzania zaznaczonym celem myszy.
     * 
     * Przechowuje cel jako bezpieczny typ EntityId (EntityHandle) z uzyciem std::optional.
     * Automatycznie resetuje zaznaczenie w przypadku usuniecia (despawn) obiektu,
     * nasluchujac zdarzen z EventBus.
     */
    class SafeTargetController
    {
    public:
        SafeTargetController()
        {
            auto& eventBus = EterBase::EventBus::GetInstance();
            
            targetDeleteSubId_ = eventBus.Subscribe<Core::Events::TargetDelete>(
                [this](const Core::Events::TargetDelete& ev) {
                    this->OnTargetDelete(ev);
                }
            );
        }

        ~SafeTargetController()
        {
            auto& eventBus = EterBase::EventBus::GetInstance();
            eventBus.Unsubscribe<Core::Events::TargetDelete>(targetDeleteSubId_);
        }

        // Blokada kopiowania i przenoszenia ze wzgledu na rejestracje w EventBus
        SafeTargetController(const SafeTargetController&) = delete;
        SafeTargetController& operator=(const SafeTargetController&) = delete;
        SafeTargetController(SafeTargetController&&) = delete;
        SafeTargetController& operator=(SafeTargetController&&) = delete;

        /**
         * @brief Ustawia nowy cel.
         * @param targetId Bezpieczny identyfikator encji.
         */
        constexpr void SetTarget(EterBase::EntityId targetId) noexcept
        {
            if (targetId.value() != 0) {
                currentTarget_ = targetId;
            } else {
                currentTarget_.reset();
            }
        }

        /**
         * @brief Zwraca aktualnie zaznaczony cel.
         * @return std::optional<EterBase::EntityId> zawierajacy ID celu lub pusty jezeli brak celu.
         */
        [[nodiscard]] constexpr std::optional<EterBase::EntityId> GetTarget() const noexcept
        {
            return currentTarget_;
        }

        /**
         * @brief Czysci aktualny cel.
         */
        constexpr void ClearTarget() noexcept
        {
            currentTarget_.reset();
        }

        /**
         * @brief Sprawdza czy kontroler posiada aktywny cel.
         * @return true jesli cel jest ustawiony, w przeciwnym razie false.
         */
        [[nodiscard]] constexpr bool HasTarget() const noexcept
        {
            return currentTarget_.has_value();
        }

    private:
        void OnTargetDelete(const Core::Events::TargetDelete& ev) noexcept
        {
            if (currentTarget_.has_value() && currentTarget_->value() == ev.targetId)
            {
                ClearTarget();
            }
        }

        std::optional<EterBase::EntityId> currentTarget_;
        uint32_t targetDeleteSubId_{0};
    };
} // namespace Client::Gameplay
