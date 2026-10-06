#pragma once

#include <expected>
#include <optional>
#include <format>
#include <cstdint>
#include <algorithm> // for std::max

#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Estimator staminy uzywany do wyliczania kosztow staminy podczas walki i ruchu.
 *        Wdrozenie oparte na standardzie C++23.
 */
class StaminaCostEstimator {
public:
    /**
     * @brief Zdarzenie uzycia staminy do wyemitowania przez EventBus.
     */
    struct StaminaUsedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;
        int32_t amount;

        StaminaUsedEvent(EterBase::EntityId id, int32_t amt) : entityId(id), amount(amt) {}
    };

    /**
     * @brief Zwraca koszt staminy za ciagly bieg.
     * @param entityId Identyfikator aktora.
     * @param distance Pokonany dystans (w jednostkach gry).
     * @return Koszt staminy (std::expected) lub blad domenowy.
     */
    [[nodiscard]] static std::expected<int32_t, EterBase::EntityError> EstimateRunCost(
        EterBase::EntityId entityId, float distance) noexcept
    {
        if (!entityId) {
            EterBase::ModernLogger::Error(
                "StaminaCostEstimator: Nieprawidlowy entityId podczas wyliczania kosztu biegu."
            );
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        if (distance <= 0.0f) {
            return 0; // Brak ruchu = brak kosztu
        }

        // Przykladowy wplyw dystansu na stamine:
        // W Metin2, stamina schodzi podczas biegu z okreslona predkoscia.
        // Tutaj zakladamy staly mnoznik dla uproszczenia (np. 1 pkt staminy na 10 jednostek dystansu).
        int32_t cost = static_cast<int32_t>(distance / 10.0f);
        
        // Zapewniamy co najmniej 1 punkt kosztu jesli byl ruch.
        int32_t finalCost = std::max<int32_t>(1, cost);

        // Emitujemy zdarzenie o poborze staminy
        UserInterface::Core::EventBus::GetInstance().Publish(StaminaUsedEvent(entityId, finalCost));

        return finalCost;
    }

    /**
     * @brief Zwraca koszt staminy za pojedynczy cios normalny lub ze skilla.
     * @param entityId Identyfikator aktora atakujacego.
     * @param skillId Uzyty skill (jesli brak, jest to zwykly cios).
     * @return Koszt staminy (std::expected) lub blad domenowy.
     */
    [[nodiscard]] static std::expected<int32_t, EterBase::EntityError> EstimateAttackCost(
        EterBase::EntityId entityId, std::optional<EterBase::SkillId> skillId) noexcept
    {
        if (!entityId) {
            EterBase::ModernLogger::Error(
                "StaminaCostEstimator: Nieprawidlowy entityId podczas wyliczania kosztu ataku."
            );
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        int32_t cost = skillId.transform([](auto sid) { return sid ? 20 : 5; }).value_or(5);

        // Emitujemy zdarzenie o poborze staminy
        UserInterface::Core::EventBus::GetInstance().Publish(StaminaUsedEvent(entityId, cost));

        return cost;
    }
};

} // namespace GameLib
