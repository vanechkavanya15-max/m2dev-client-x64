#pragma once

#include <expected>
#include <chrono>
#include "CombatDomain.h"
#include "CombatError.h"

namespace Client::Gameplay {

// Kontekst ataku, przechowuje wszystkie parametry niezbedne do walidacji ciosu.
struct AttackContext {
    Position3D attackerPos;
    float attackerRotY = 0.0f;
    Position3D targetPos;
    Box3D targetBox;
    WeaponType weapon;
    bool isTargetDead = false;
    bool isAttackerStunned = false;
    std::chrono::steady_clock::time_point lastAttackTime;
    std::chrono::steady_clock::time_point currentTime;
    std::chrono::milliseconds attackCooldown;
};

// Klasa odpowiedzialna za weryfikacje mozliwosci zadania ciosu.
// Implementacja w architekturze AI-First (stateless, C++23).
class CombatAttackEvaluator {
public:
    // Weryfikuje kontekst ataku. Zwraca sukces lub szczegolowy blad.
    [[nodiscard]] static std::expected<void, CombatError> Evaluate(const AttackContext& context) noexcept {
        
        // 1. Sprawdzenie czy cel zyje
        if (context.isTargetDead) {
            return std::unexpected(CombatError::TargetDead);
        }

        // 2. Sprawdzenie czy atakujacy jest ogluszony
        if (context.isAttackerStunned) {
            return std::unexpected(CombatError::CharacterStunned);
        }

        // 3. Sprawdzenie cooldownu ataku
        if (context.lastAttackTime + context.attackCooldown > context.currentTime) {
            return std::unexpected(CombatError::AttackCooldown);
        }

        // 4. Sprawdzenie zasiegu i kata uderzenia (zalezy od broni)
        // CheckTargetHit uzywa funkcji nalezacych do domenowej warstwy obliczeniowej.
        if (!AttackGeometry::CheckTargetHit(context.attackerPos, context.attackerRotY, context.targetPos, context.targetBox, context.weapon)) {
             return std::unexpected(CombatError::TargetOutOfRange);
        }

        return {};
    }
};

} // namespace Client::Gameplay
