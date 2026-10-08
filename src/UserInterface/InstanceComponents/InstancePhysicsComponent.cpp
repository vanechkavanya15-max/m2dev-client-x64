#include "StdAfx.h"
#include "InstancePhysicsComponent.h"
#include <cmath>

namespace UserInterface::InstanceComponents {

InstancePhysicsComponent::InstancePhysicsComponent(CActorInstance& actorInstance)
    : m_actorInstance(actorInstance)
{
}

void InstancePhysicsComponent::SetPixelPosition(const TPixelPosition& pos)
{
    m_pixelPosition = pos;
    m_actorInstance.SetPixelPosition(pos);
}

const TPixelPosition& InstancePhysicsComponent::GetPixelPosition() const noexcept
{
    return m_actorInstance.NEW_GetCurPixelPositionRef();
}

TPixelPosition& InstancePhysicsComponent::GetPixelPositionRef() noexcept
{
    return const_cast<TPixelPosition&>(m_actorInstance.NEW_GetCurPixelPositionRef());
}

void InstancePhysicsComponent::SetRotation(float rotation)
{
    m_rotation = rotation;
    m_targetRotation = rotation;
    m_actorInstance.SetRotation(rotation);
}

void InstancePhysicsComponent::SetTargetRotation(float targetRotation)
{
    m_targetRotation = targetRotation;
    m_actorInstance.SetAdvancingRotation(targetRotation);
}

float InstancePhysicsComponent::CalculateDistanceSq3d(const TPixelPosition& targetPos) const noexcept
{
    const auto& cur = GetPixelPosition();
    float dx = cur.x - targetPos.x;
    float dy = cur.y - targetPos.y;
    float dz = cur.z - targetPos.z;
    return dx * dx + dy * dy + dz * dz;
}

float InstancePhysicsComponent::CalculateDistance2d(const TPixelPosition& targetPos) const noexcept
{
    const auto& cur = GetPixelPosition();
    float dx = cur.x - targetPos.x;
    float dy = cur.y - targetPos.y;
    return std::sqrt(dx * dx + dy * dy);
}

void InstancePhysicsComponent::UpdateMovement()
{
    m_pixelPosition = m_actorInstance.NEW_GetCurPixelPositionRef();
    m_rotation = m_actorInstance.GetRotation();
}

void InstancePhysicsComponent::Clear()
{
    m_pixelPosition = TPixelPosition{0.0f, 0.0f, 0.0f};
    m_rotation = 0.0f;
    m_targetRotation = 0.0f;
    m_rotationSpeed = 24.0f;
    m_moveSpeed = 100;
}

} // namespace UserInterface::InstanceComponents
