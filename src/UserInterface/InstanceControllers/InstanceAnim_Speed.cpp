#include "../StdAfx.h"
#include "IInstanceAnimationController.h"
#include "Core/EventBus.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"

namespace UserInterface::InstanceControllers {

/**
 * @brief Zdarzenie emitowane po udanej aktualizacji szybkosci animacji dla aktora.
 */
struct AnimSpeedUpdatedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    float newSpeed;
    MotionState currentState;

    explicit AnimSpeedUpdatedEvent(EterBase::EntityId id, float speed, MotionState state)
        : entityId(id), newSpeed(speed), currentState(state) {}
};

/**
 * @brief Handler zarzadzajacy predkoscia animacji (attack speed / movement speed).
 * Realizuje funkcjonalnosc poza-klasowo zachowujac Zero-Conflict z interfejsem.
 */
class InstanceAnimSpeedHandler {
public:
    /**
     * @brief Skaluje predkosc animacji instancji na podstawie obecnego stanu i modyfikatorow.
     * @param controller Referencja do kontrolera animacji (dependency injection)
     * @param entityId Identyfikator modyfikowanej instancji (do zdarzenia)
     * @param attackSpeedMultiplier Skaler predkosci ataku (np. 1.25f to +25% attack speed)
     * @param movementSpeedMultiplier Skaler predkosci poruszania sie (np. 1.50f to +50% move speed)
     * @return Sukces (void) lub blad domenowy, jesli operacja byla niemozliwa
     */
    static std::expected<void, EterBase::EntityError> UpdateAnimationSpeed(
        IInstanceAnimationController& controller,
        EterBase::EntityId entityId,
        float attackSpeedMultiplier,
        float movementSpeedMultiplier) 
    {
        if (!entityId) {
            EterBase::ModernLogger::Error("InstanceAnimSpeedHandler: Otrzymano nieprawidlowy EntityId.");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        const MotionState state = controller.GetMotionState();
        float targetSpeed = 1.0f;

        switch (state) {
            case MotionState::Walk:
            case MotionState::Run:
                targetSpeed = movementSpeedMultiplier;
                break;
            case MotionState::Attack:
            case MotionState::Skill:
                targetSpeed = attackSpeedMultiplier;
                break;
            case MotionState::Idle:
            case MotionState::Damaged:
            case MotionState::Dead:
                // Te stany zazwyczaj nie reaguja na statystyki, odtwarzane ze stala szybkoscia.
                targetSpeed = 1.0f;
                break;
            default:
                EterBase::ModernLogger::Warning("InstanceAnimSpeedHandler: Nieznany stan animacji.");
                targetSpeed = 1.0f;
                break;
        }

        // Aplikacja predkosci na kontroler
        controller.SetMotionSpeed(targetSpeed);

        EterBase::ModernLogger::Debug("InstanceAnimSpeedHandler: Ustawiono predkosc {} dla EntityId {} w stanie {}", 
            targetSpeed, static_cast<uint32_t>(entityId.value()), static_cast<uint8_t>(state));

        // Powiadomienie innych systemow o zmianie
        UserInterface::Core::EventBus::GetInstance().Publish(AnimSpeedUpdatedEvent(entityId, targetSpeed, state));

        return {}; // std::expected
    }
};

} // namespace UserInterface::InstanceControllers
