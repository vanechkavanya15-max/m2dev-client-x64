#pragma once

#include <cstdint>
#include <string_view>
#include <expected>
#include <optional>
#include <format>
#include <span>
#include <string>

#include "src/EterBase/StrongTypes.h"
#include "src/UserInterface/Core/EventBus.h"

namespace UserInterface::Core::CombatEvents {

/**
 * @brief Enum reprezentujacy mozliwe bledy podczas tworzenia zdarzen walki.
 */
enum class CombatError {
    InvalidAttacker,
    InvalidVictim,
    InvalidDamageAmount,
    SelfHarmNotAllowed
};

/**
 * @brief Konwertuje blad na czytelna dla czlowieka wiadomosc.
 * 
 * @param error Kod bledu z wyliczenia CombatError.
 * @return C-string z krotkim opisem bledu.
 */
constexpr std::string_view CombatErrorToString(CombatError error) noexcept {
    switch (error) {
        case CombatError::InvalidAttacker: return "Attacker EntityId cannot be zero.";
        case CombatError::InvalidVictim: return "Victim EntityId cannot be zero.";
        case CombatError::InvalidDamageAmount: return "Damage amount must be strictly positive.";
        case CombatError::SelfHarmNotAllowed: return "Attacker and Victim cannot be the same entity.";
        default: return "Unknown combat error.";
    }
}

/**
 * @brief Typ ataku uzywany w systemie zdarzen.
 */
enum class AttackType {
    Melee,
    Ranged,
    Magic,
    Splash
};

/**
 * @brief Zdarzenie emitowane, gdy jeden aktor (Attacker) zada obrazenia drugiemu (Victim).
 * 
 * Dziedziczy po IEvent, aby moglo byc przeslane przez EventBus.
 */
struct ActorDamaged : public IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId victimId;
    uint32_t damageAmount;
    AttackType attackType;
    std::optional<EterBase::SkillId> usedSkill;
    bool isCritical;
    bool isPiercing;

private:
    /**
     * @brief Prywatny konstruktor zapobiegajacy utworzeniu niepoprawnego zdarzenia.
     */
    ActorDamaged(EterBase::EntityId attacker, EterBase::EntityId victim, uint32_t damage, 
                 AttackType type, std::optional<EterBase::SkillId> skill, 
                 bool crit, bool pierce)
        : attackerId(attacker), victimId(victim), damageAmount(damage), 
          attackType(type), usedSkill(skill), isCritical(crit), isPiercing(pierce) {}

public:
    /**
     * @brief Zwraca sformatowany opis zdarzenia. Wykorzystuje std::format.
     * @return std::string Opis zdarzenia obrazen.
     */
    [[nodiscard]] std::string ToString() const {
        return std::format("ActorDamaged: Attacker [{}] hit Victim [{}] for {} damage. Crit: {}, Pierce: {}. Skill: {}",
            attackerId.value(), victimId.value(), damageAmount, isCritical, isPiercing,
            usedSkill.transform([](auto s) { return std::to_string(s.value()); }).value_or("None"));
    }

    /**
     * @brief Bezpieczna fabryka zwracajaca poprawne zdarzenie ActorDamaged lub blad.
     * 
     * @param attacker Identyfikator atakujacego.
     * @param victim Identyfikator ofiary.
     * @param damage Ilosc zadanych obrazen.
     * @param type Rodzaj ataku (Melee, Ranged, etc.).
     * @param skill Opcjonalny identyfikator umiejetnosci.
     * @param crit Czy trafienie bylo krytyczne.
     * @param pierce Czy trafienie bylo przeszywajace.
     * @return std::expected<ActorDamaged, CombatError> Zdarzenie lub blad walidacji.
     */
    static std::expected<ActorDamaged, CombatError> Create(
        EterBase::EntityId attacker,
        EterBase::EntityId victim,
        uint32_t damage,
        AttackType type,
        std::optional<EterBase::SkillId> skill = std::nullopt,
        bool crit = false,
        bool pierce = false
    ) {
        if (!attacker) return std::unexpected(CombatError::InvalidAttacker);
        if (!victim) return std::unexpected(CombatError::InvalidVictim);
        if (attacker == victim) return std::unexpected(CombatError::SelfHarmNotAllowed);
        if (damage == 0) return std::unexpected(CombatError::InvalidDamageAmount);

        return ActorDamaged(attacker, victim, damage, type, skill, crit, pierce);
    }
};

/**
 * @brief Zdarzenie emitowane, gdy jakikolwiek aktor umiera po otrzymaniu smiertelnego ciosu.
 */
struct TargetDied : public IEvent {
    EterBase::EntityId targetId;
    std::optional<EterBase::EntityId> killerId;

private:
    TargetDied(EterBase::EntityId target, std::optional<EterBase::EntityId> killer)
        : targetId(target), killerId(killer) {}

public:
    /**
     * @brief Zwraca sformatowany opis smierci aktora.
     * @return std::string Reprezentacja zdarzenia.
     */
    [[nodiscard]] std::string ToString() const {
        return std::format("TargetDied: Target [{}] was killed by Killer [{}]", 
            targetId.value(), 
            killerId.transform([](auto k) { return std::to_string(k.value()); }).value_or("Environment"));
    }

    /**
     * @brief Bezpieczna fabryka do utworzenia zdarzenia smierci.
     * 
     * @param target Identyfikator umarlego.
     * @param killer Opcjonalny identyfikator zabojcy.
     * @return std::expected<TargetDied, CombatError> Utworzone zdarzenie lub blad.
     */
    static std::expected<TargetDied, CombatError> Create(
        EterBase::EntityId target,
        std::optional<EterBase::EntityId> killer = std::nullopt
    ) {
        if (!target) return std::unexpected(CombatError::InvalidVictim);
        if (killer.has_value() && target == killer.value()) {
            return std::unexpected(CombatError::SelfHarmNotAllowed);
        }

        return TargetDied(target, killer);
    }
};

/**
 * @brief Zdarzenie emitowane, gdy aktor uderzy krytycznie w ofiare. 
 * Moze byc podpinane przez system efektow wizualnych.
 */
struct CriticalHitLanded : public IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId victimId;
    uint32_t damageMultiplier;

private:
    CriticalHitLanded(EterBase::EntityId attacker, EterBase::EntityId victim, uint32_t multiplier)
        : attackerId(attacker), victimId(victim), damageMultiplier(multiplier) {}

public:
    /**
     * @brief Zwraca sformatowany log krytycznego ciosu.
     * @return std::string Reprezentacja ciosu.
     */
    [[nodiscard]] std::string ToString() const {
        return std::format("CriticalHitLanded: Attacker [{}] critically hit Victim [{}] with {}x multiplier.",
            attackerId.value(), victimId.value(), damageMultiplier);
    }

    /**
     * @brief Bezpieczne tworzenie zdarzenia trafienia krytycznego.
     * 
     * @param attacker Identyfikator postaci atakujacej.
     * @param victim Identyfikator postaci otrzymujacej cios.
     * @param multiplier Mnoznik obrazen krytycznych (domyslnie 2x).
     * @return std::expected<CriticalHitLanded, CombatError> Obiekt zdarzenia lub kod bledu.
     */
    static std::expected<CriticalHitLanded, CombatError> Create(
        EterBase::EntityId attacker,
        EterBase::EntityId victim,
        uint32_t multiplier = 2
    ) {
        if (!attacker) return std::unexpected(CombatError::InvalidAttacker);
        if (!victim) return std::unexpected(CombatError::InvalidVictim);
        if (attacker == victim) return std::unexpected(CombatError::SelfHarmNotAllowed);
        if (multiplier < 1) return std::unexpected(CombatError::InvalidDamageAmount);

        return CriticalHitLanded(attacker, victim, multiplier);
    }
};

} // namespace UserInterface::Core::CombatEvents
