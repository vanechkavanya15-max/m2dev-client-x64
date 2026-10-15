#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <span>
#include <string_view>

#include "EterBase/EventBus.h"
#include "EterBase/StrongTypes.h"

namespace Client::Events {

/**
 * @brief Typ bledu walki.
 */
enum class CombatError : uint8_t {
    None = 0,
    InvalidTarget,
    OutOfRange,
    SkillOnCooldown,
    NotEnoughMana,
    TargetInvulnerable,
    Unknown
};

/**
 * @brief Rezultat operacji walki (C++23 std::expected).
 */
template <typename T>
using CombatResult = std::expected<T, CombatError>;

/**
 * @brief Zdarzenie zadania obrazen (Damage).
 */
struct CombatDamageEvent final : public ::EterBase::IEvent {
    ::EterBase::EntityId attackerId;
    ::EterBase::EntityId targetId;
    uint32_t damageAmount;
    bool isCritical;
    bool isPoison;

    constexpr CombatDamageEvent(
        ::EterBase::EntityId attacker,
        ::EterBase::EntityId target,
        uint32_t damage,
        bool critical = false,
        bool poison = false) noexcept
        : attackerId(attacker),
          targetId(target),
          damageAmount(damage),
          isCritical(critical),
          isPoison(poison) {}
};

/**
 * @brief Zdarzenie leczenia (Heal).
 */
struct CombatHealEvent final : public ::EterBase::IEvent {
    ::EterBase::EntityId healerId;
    ::EterBase::EntityId targetId;
    uint32_t healAmount;

    constexpr CombatHealEvent(
        ::EterBase::EntityId healer,
        ::EterBase::EntityId target,
        uint32_t heal) noexcept
        : healerId(healer),
          targetId(target),
          healAmount(heal) {}
};

/**
 * @brief Zdarzenie smierci podczas walki (Death).
 */
struct CombatDeathEvent final : public ::EterBase::IEvent {
    ::EterBase::EntityId victimId;
    std::optional<::EterBase::EntityId> killerId;

    constexpr CombatDeathEvent(
        ::EterBase::EntityId victim,
        std::optional<::EterBase::EntityId> killer = std::nullopt) noexcept
        : victimId(victim),
          killerId(killer) {}
};

/**
 * @brief Zdarzenie aktywacji umiejetnosci (Skill).
 */
struct CombatSkillEvent final : public ::EterBase::IEvent {
    ::EterBase::EntityId casterId;
    ::EterBase::SkillId skillId;
    std::optional<::EterBase::EntityId> targetId;
    uint8_t skillLevel;

    constexpr CombatSkillEvent(
        ::EterBase::EntityId caster,
        ::EterBase::SkillId skill,
        uint8_t level,
        std::optional<::EterBase::EntityId> target = std::nullopt) noexcept
        : casterId(caster),
          skillId(skill),
          targetId(target),
          skillLevel(level) {}
};

} // namespace Client::Events
