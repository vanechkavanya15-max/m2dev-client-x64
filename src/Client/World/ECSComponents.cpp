#include "ECSComponents.h"

namespace Client::World {

// TransformComponent
TransformComponent::TransformComponent() {
    Reset();
}

void TransformComponent::Reset() {
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    rot = 0.0f;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            worldMatrix.m[i][j] = (i == j) ? 1.0f : 0.0f; // Identity matrix
        }
    }
}

// VisualComponent
VisualComponent::VisualComponent() {
    Reset();
}

void VisualComponent::Reset() {
    race = RaceVnum{0};
    alpha = 1.0f;
    isVisible = true;
    armor = ItemVnum{0};
    weapon = ItemVnum{0};
}

// MotionComponent
MotionComponent::MotionComponent() {
    Reset();
}

void MotionComponent::Reset() {
    motionMode = 0;
    motionIndex = 0;
    speed = 1.0f;
    loopTime = 0.0f;
}

// CombatStateComponent
CombatStateComponent::CombatStateComponent() {
    Reset();
}

void CombatStateComponent::Reset() {
    curHP = 0;
    maxHP = 0;
    targetVid = EntityVid{0};
    isDead = false;
}

} // namespace Client::World
