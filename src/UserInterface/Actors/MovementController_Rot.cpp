#include "../StdAfx.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../Domain/RotationHelper.h"

#include <cmath>

namespace UserInterface::Actors {

/**
 * @brief Interpolates between two angles using spherical linear interpolation (Slerp).
 * 
 * Slerp calculates the shortest path between two angles (in degrees) and interpolates
 * smoothly along this path.
 * 
 * @param currentAngle The current angle in degrees.
 * @param targetAngle The target angle in degrees.
 * @param t The interpolation factor [0.0, 1.0].
 * @return EterBase::PacketResult<float> The interpolated angle in degrees, normalized to [0, 360).
 */
EterBase::PacketResult<float> MovementRotationSlerp(float currentAngle, float targetAngle, float t)
{
    // Ensure t is clamped between 0.0 and 1.0
    t = std::fmax(0.0f, std::fmin(1.0f, t));

    // Calculate the shortest signed difference between the two angles
    float difference = Movement::RotationHelper::GetSignedAngleDifference(currentAngle, targetAngle);

    // Apply the interpolation factor to the difference
    float interpolatedDifference = difference * t;

    float newAngle = Movement::RotationHelper::NormalizeAngle(currentAngle + interpolatedDifference);
    
    // Log calculation debug information using ModernLogger
    EterBase::ModernLogger::Debug("MovementRotationSlerp: cur={}, target={}, t={}, result={}", 
        currentAngle, targetAngle, t, newAngle);
        
    return newAngle;
}

/**
 * @brief Custom Event for when rotation reaches target.
 */
struct ActorRotationTargetReachedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    float finalRotation;
    
    ActorRotationTargetReachedEvent(EterBase::EntityId id, float rotation) 
        : entityId(id), finalRotation(rotation) {}
};

/**
 * @brief Controller class for rotating an actor.
 */
class MovementController_Rot {
public:
    /**
     * @brief Creates a new controller for the given actor.
     * @param id The ID of the actor.
     * @param currentRot The starting rotation.
     */
    MovementController_Rot(EterBase::EntityId id, float currentRot)
        : m_entityId(id), m_currentRotation(currentRot), m_targetRotation(currentRot), m_isRotating(false)
    {
    }

    /**
     * @brief Sets a new target rotation.
     * @param targetRot The target rotation in degrees.
     */
    void SetTargetRotation(float targetRot) {
        m_targetRotation = Movement::RotationHelper::NormalizeAngle(targetRot);
        m_isRotating = true;
        EterBase::ModernLogger::Info("MovementController_Rot: Setting target rotation to {} for EntityId {}", 
            m_targetRotation, m_entityId.value());
    }

    /**
     * @brief Updates the rotation using Slerp.
     * @param t The interpolation factor, generally representing time delta multiplied by speed.
     * @return EterBase::PacketResult<float> Expected returning the new rotation.
     */
    EterBase::PacketResult<float> Update(float t) {
        if (!m_isRotating) {
            return m_currentRotation;
        }

        auto result = MovementRotationSlerp(m_currentRotation, m_targetRotation, t);
        if (!result) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_currentRotation = *result;

        // Check if we have reached the target
        if (std::abs(Movement::RotationHelper::GetSignedAngleDifference(m_currentRotation, m_targetRotation)) < 0.1f) {
            m_currentRotation = m_targetRotation;
            m_isRotating = false;
            
            // Publish event using EventBus
            Core::EventBus::GetInstance().Publish(ActorRotationTargetReachedEvent(m_entityId, m_currentRotation));
        }

        return m_currentRotation;
    }

private:
    EterBase::EntityId m_entityId;
    float m_currentRotation;
    float m_targetRotation;
    bool m_isRotating;
};

} // namespace UserInterface::Actors
