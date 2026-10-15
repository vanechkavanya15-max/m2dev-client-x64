#pragma once

#include <cstdint>
#include "../../EterBase/StrongTypes.h"

namespace Client::World {

// Podstawowe typy dla komponentow ECS

struct MapCoords {
    float x;
    float y;
    float z;
};

struct Mat4 {
    float m[4][4];
};

struct RaceVnumTag {};
using RaceVnum = EterBase::StrongType<RaceVnumTag, uint32_t, 0>;
using EntityVid = EterBase::EntityId;
using ItemVnum = EterBase::ItemVnum;

// ZASADA 2: Struktury komponentow oparte na Data-Oriented Design

struct TransformComponent {
    MapCoords pos;
    float rot;
    Mat4 worldMatrix;

    TransformComponent();
    void Reset();
};

struct VisualComponent {
    RaceVnum race;
    float alpha;
    bool isVisible;
    ItemVnum armor;
    ItemVnum weapon;

    VisualComponent();
    void Reset();
};

struct MotionComponent {
    uint16_t motionMode;
    uint16_t motionIndex;
    float speed;
    float loopTime;

    MotionComponent();
    void Reset();
};

struct CombatStateComponent {
    int32_t curHP;
    int32_t maxHP;
    EntityVid targetVid;
    bool isDead;

    CombatStateComponent();
    void Reset();
};

} // namespace Client::World
