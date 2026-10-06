#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <string_view>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Event triggered when the player's statistics are updated.
 * 
 * Used to notify the UI to refresh without direct coupling.
 */
struct PlayerStatsUpdatedEvent : public Core::IEvent {
    EterBase::EntityId playerId;

    /**
     * @brief Constructs the event with a specific player ID.
     * @param id The unique identifier of the player whose stats changed.
     */
    explicit PlayerStatsUpdatedEvent(EterBase::EntityId id) : playerId(id) {}
};

/**
 * @brief Enumeration of primary player statistics.
 */
enum class StatType : uint8_t {
    Strength = 0,
    Vitality,
    Dexterity,
    Intelligence
};

/**
 * @brief Enumeration of speed types.
 */
enum class SpeedType : uint8_t {
    Movement = 0,
    Attack,
    Magic
};

/**
 * @brief Pure domain model for player statistics.
 *
 * Enforces C++23 standards, Zero-Conflict policy, and decoupling from Python GUI.
 * Uses StrongTypes and Result/Expected for safe error handling.
 */
class PlayerStatsModel {
public:
    /**
     * @brief Default constructor.
     */
    PlayerStatsModel() = default;

    /**
     * @brief Constructor with an assigned player ID.
     * @param id The strong type ID for the entity.
     */
    explicit PlayerStatsModel(EterBase::EntityId id) : playerId(id) {}

    /**
     * @brief Retrieves the player's EntityId.
     * @return The EntityId.
     */
    [[nodiscard]] EterBase::EntityId GetPlayerId() const noexcept {
        return playerId;
    }

    /**
     * @brief Sets the player's EntityId.
     * @param id The new EntityId.
     */
    void SetPlayerId(EterBase::EntityId id) noexcept {
        playerId = id;
    }

    /**
     * @brief Sets a primary stat (STR, VIT, DEX, INT).
     * @param type The type of stat to modify.
     * @param value The new stat value.
     * @return VoidResult indicating success or an EntityError if the type is invalid.
     */
    EterBase::VoidResult<EterBase::EntityError> SetStat(StatType type, uint16_t value) {
        switch (type) {
            case StatType::Strength:     strength = value; break;
            case StatType::Vitality:     vitality = value; break;
            case StatType::Dexterity:    dexterity = value; break;
            case StatType::Intelligence: intelligence = value; break;
            default: return EterBase::MakeError(EterBase::EntityError::InvalidType);
        }
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Retrieves a primary stat (STR, VIT, DEX, INT).
     * @param type The type of stat to retrieve.
     * @return The stat value or an EntityError if the type is invalid.
     */
    EterBase::Result<uint16_t, EterBase::EntityError> GetStat(StatType type) const {
        switch (type) {
            case StatType::Strength:     return strength;
            case StatType::Vitality:     return vitality;
            case StatType::Dexterity:    return dexterity;
            case StatType::Intelligence: return intelligence;
            default: return EterBase::MakeError(EterBase::EntityError::InvalidType);
        }
    }

    /**
     * @brief Sets a speed stat (Movement, Attack, Magic).
     * @param type The type of speed to modify.
     * @param value The new speed value.
     * @return VoidResult indicating success or an EntityError if the type is invalid.
     */
    EterBase::VoidResult<EterBase::EntityError> SetSpeed(SpeedType type, uint16_t value) {
        switch (type) {
            case SpeedType::Movement: movementSpeed = value; break;
            case SpeedType::Attack:   attackSpeed = value; break;
            case SpeedType::Magic:    magicSpeed = value; break;
            default: return EterBase::MakeError(EterBase::EntityError::InvalidType);
        }
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Retrieves a speed stat (Movement, Attack, Magic).
     * @param type The type of speed to retrieve.
     * @return The speed value or an EntityError if the type is invalid.
     */
    EterBase::Result<uint16_t, EterBase::EntityError> GetSpeed(SpeedType type) const {
        switch (type) {
            case SpeedType::Movement: return movementSpeed;
            case SpeedType::Attack:   return attackSpeed;
            case SpeedType::Magic:    return magicSpeed;
            default: return EterBase::MakeError(EterBase::EntityError::InvalidType);
        }
    }

    /**
     * @brief Updates multiple primary stats safely using std::optional.
     * 
     * Uses monadic .value_or() to apply new values without pyramid if-statements.
     * @param optStr Optional strength value.
     * @param optVit Optional vitality value.
     * @param optDex Optional dexterity value.
     * @param optInt Optional intelligence value.
     */
    void UpdateStats(std::optional<uint16_t> optStr, std::optional<uint16_t> optVit,
                     std::optional<uint16_t> optDex, std::optional<uint16_t> optInt) {
        
        strength     = optStr.value_or(strength);
        vitality     = optVit.value_or(vitality);
        dexterity    = optDex.value_or(dexterity);
        intelligence = optInt.value_or(intelligence);
        
        EterBase::ModernLogger::Info("Updated multiple primary stats for player ID: {}", playerId.value());
        PublishUpdateEvent();
    }

    /**
     * @brief Updates multiple speed stats safely using std::optional.
     * @param optMovement Optional movement speed value.
     * @param optAttack Optional attack speed value.
     * @param optMagic Optional magic speed value.
     */
    void UpdateSpeeds(std::optional<uint16_t> optMovement, std::optional<uint16_t> optAttack,
                      std::optional<uint16_t> optMagic) {
        
        movementSpeed = optMovement.value_or(movementSpeed);
        attackSpeed   = optAttack.value_or(attackSpeed);
        magicSpeed    = optMagic.value_or(magicSpeed);

        EterBase::ModernLogger::Info("Updated speed stats for player ID: {}", playerId.value());
        PublishUpdateEvent();
    }

    // Explicit direct accessors for convenience
    
    /**
     * @brief Returns the raw strength value.
     * @return The strength.
     */
    [[nodiscard]] uint16_t GetStrength() const noexcept { return strength; }
    
    /**
     * @brief Returns the raw vitality value.
     * @return The vitality.
     */
    [[nodiscard]] uint16_t GetVitality() const noexcept { return vitality; }
    
    /**
     * @brief Returns the raw dexterity value.
     * @return The dexterity.
     */
    [[nodiscard]] uint16_t GetDexterity() const noexcept { return dexterity; }
    
    /**
     * @brief Returns the raw intelligence value.
     * @return The intelligence.
     */
    [[nodiscard]] uint16_t GetIntelligence() const noexcept { return intelligence; }

    /**
     * @brief Returns the raw movement speed value.
     * @return The movement speed.
     */
    [[nodiscard]] uint16_t GetMovementSpeed() const noexcept { return movementSpeed; }
    
    /**
     * @brief Returns the raw attack speed value.
     * @return The attack speed.
     */
    [[nodiscard]] uint16_t GetAttackSpeed() const noexcept { return attackSpeed; }
    
    /**
     * @brief Returns the raw magic speed value.
     * @return The magic speed.
     */
    [[nodiscard]] uint16_t GetMagicSpeed() const noexcept { return magicSpeed; }

private:
    /**
     * @brief Emits a PlayerStatsUpdatedEvent via the central EventBus.
     */
    void PublishUpdateEvent() const {
        Core::EventBus::GetInstance().Publish(PlayerStatsUpdatedEvent{playerId});
    }

    EterBase::EntityId playerId{}; ///< Unique identifier for the associated player entity.

    uint16_t strength{0};     ///< Physical strength stat.
    uint16_t vitality{0};     ///< Vitality stat.
    uint16_t dexterity{0};    ///< Dexterity stat.
    uint16_t intelligence{0}; ///< Intelligence (Magic) stat.

    uint16_t movementSpeed{100}; ///< Base movement speed.
    uint16_t attackSpeed{100};   ///< Base attack speed.
    uint16_t magicSpeed{100};    ///< Base casting speed.
};

} // namespace UserInterface::Domain
