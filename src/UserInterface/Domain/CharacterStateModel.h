#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <functional>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Core/Events.h"
#include "PositionModel.h"

namespace UserInterface::Domain
{
    /**
     * @brief Represents the combat flags for a character.
     */
    struct CombatFlags
    {
        bool isDead = false;
        bool isInCombat = false;
        bool isStunned = false;
    };

    /**
     * @brief Domain-specific event emitted when a character's HP changes.
     */
    struct DomainHpChangeEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        uint32_t currentHp;
        uint32_t maxHp;

        DomainHpChangeEvent(EterBase::EntityId id_, uint32_t currentHp_, uint32_t maxHp_)
            : id(id_), currentHp(currentHp_), maxHp(maxHp_) {}
    };

    /**
     * @brief Domain-specific event emitted when a character's MP changes.
     */
    struct DomainMpChangeEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        uint32_t currentMp;
        uint32_t maxMp;

        DomainMpChangeEvent(EterBase::EntityId id_, uint32_t currentMp_, uint32_t maxMp_)
            : id(id_), currentMp(currentMp_), maxMp(maxMp_) {}
    };

    /**
     * @brief Domain-specific event emitted when a character's EXP changes.
     */
    struct DomainExpChangeEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        uint64_t exp;

        DomainExpChangeEvent(EterBase::EntityId id_, uint64_t exp_)
            : id(id_), exp(exp_) {}
    };

    /**
     * @brief Domain-specific event emitted when a character's position changes.
     */
    struct DomainPositionChangeEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        std::optional<GlobalPosition> position;

        DomainPositionChangeEvent(EterBase::EntityId id_, std::optional<GlobalPosition> position_)
            : id(id_), position(position_) {}
    };

    /**
     * @brief Domain-specific event emitted when a character's combat flags change.
     */
    struct DomainCombatFlagsChangeEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId id;
        CombatFlags flags;

        DomainCombatFlagsChangeEvent(EterBase::EntityId id_, CombatFlags flags_)
            : id(id_), flags(flags_) {}
    };

    /**
     * @brief Clean model representing the state of a character (HP, MP, EXP, Position, Combat Flags).
     *        Follows C++23 standards, avoids Hungarian notation, and decouples GUI by emitting events.
     */
    class CharacterStateModel
    {
    public:
        /**
         * @brief Constructs a new CharacterStateModel with the given entity ID.
         * @param id The unique entity ID of the character.
         */
        explicit CharacterStateModel(EterBase::EntityId id) : id_(id) {}

        /**
         * @brief Gets the entity ID of the character.
         * @return The entity ID.
         */
        [[nodiscard]] EterBase::EntityId GetId() const noexcept
        {
            return id_;
        }

        /**
         * @brief Sets the HP of the character.
         * @param currentHp The new current HP.
         * @param maxHp The new maximum HP.
         * @return A PacketResult indicating success or failure.
         */
        EterBase::PacketResult<void> SetHp(uint32_t currentHp, uint32_t maxHp)
        {
            if (currentHp > maxHp)
            {
                EterBase::ModernLogger::Error("Failed to set HP for Entity {}: currentHp ({}) exceeds maxHp ({})", id_.value(), currentHp, maxHp);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            hp_ = currentHp;
            maxHp_ = maxHp;
            combatFlags_.isDead = (hp_ == 0);

            EterBase::ModernLogger::Info("Entity {} HP updated: {} / {}", id_.value(), hp_, maxHp_);

            DomainHpChangeEvent hpEvent{ id_, hp_, maxHp_ };
            UserInterface::Core::EventBus::GetInstance().Publish(hpEvent);

            return {};
        }

        /**
         * @brief Gets the current HP of the character.
         * @return The current HP.
         */
        [[nodiscard]] uint32_t GetHp() const noexcept
        {
            return hp_;
        }

        /**
         * @brief Gets the max HP of the character.
         * @return The max HP.
         */
        [[nodiscard]] uint32_t GetMaxHp() const noexcept
        {
            return maxHp_;
        }

        /**
         * @brief Sets the MP of the character.
         * @param currentMp The new current MP.
         * @param maxMp The new maximum MP.
         * @return A PacketResult indicating success or failure.
         */
        EterBase::PacketResult<void> SetMp(uint32_t currentMp, uint32_t maxMp)
        {
            if (currentMp > maxMp)
            {
                EterBase::ModernLogger::Error("Failed to set MP for Entity {}: currentMp ({}) exceeds maxMp ({})", id_.value(), currentMp, maxMp);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            mp_ = currentMp;
            maxMp_ = maxMp;

            EterBase::ModernLogger::Info("Entity {} MP updated: {} / {}", id_.value(), mp_, maxMp_);
            
            DomainMpChangeEvent mpEvent{ id_, mp_, maxMp_ };
            UserInterface::Core::EventBus::GetInstance().Publish(mpEvent);

            return {};
        }

        /**
         * @brief Gets the current MP of the character.
         * @return The current MP.
         */
        [[nodiscard]] uint32_t GetMp() const noexcept
        {
            return mp_;
        }

        /**
         * @brief Gets the max MP of the character.
         * @return The max MP.
         */
        [[nodiscard]] uint32_t GetMaxMp() const noexcept
        {
            return maxMp_;
        }

        /**
         * @brief Sets the experience points of the character.
         * @param exp The new experience points.
         * @return A PacketResult indicating success or failure.
         */
        EterBase::PacketResult<void> SetExp(uint64_t exp)
        {
            exp_ = exp;
            EterBase::ModernLogger::Info("Entity {} EXP updated: {}", id_.value(), exp_);
            
            DomainExpChangeEvent expEvent{ id_, exp_ };
            UserInterface::Core::EventBus::GetInstance().Publish(expEvent);

            return {};
        }

        /**
         * @brief Gets the experience points of the character.
         * @return The experience points.
         */
        [[nodiscard]] uint64_t GetExp() const noexcept
        {
            return exp_;
        }

        /**
         * @brief Sets the position of the character.
         * @param position The new global position.
         * @return A PacketResult indicating success or failure.
         */
        EterBase::PacketResult<void> SetPosition(const GlobalPosition& position)
        {
            position_ = position;
            EterBase::ModernLogger::Info("Entity {} Position updated: ({}, {})", id_.value(), position.x, position.y);
            
            DomainPositionChangeEvent posEvent{ id_, position_ };
            UserInterface::Core::EventBus::GetInstance().Publish(posEvent);

            return {};
        }

        /**
         * @brief Gets the position of the character, if set.
         * @return An optional containing the global position, or std::nullopt.
         */
        [[nodiscard]] std::optional<GlobalPosition> GetPosition() const noexcept
        {
            return position_;
        }

        /**
         * @brief Modifies the position using a transformation function if the position is set.
         * @param transformFunc The transformation function.
         */
        template <typename F>
        void TransformPosition(F&& transformFunc)
        {
            position_ = position_.transform(std::forward<F>(transformFunc));
        }

        /**
         * @brief Gets the position or a fallback default position.
         * @param defaultPosition The fallback position.
         * @return The current position if set, otherwise the fallback position.
         */
        [[nodiscard]] GlobalPosition GetPositionOr(const GlobalPosition& defaultPosition) const noexcept
        {
            return position_.value_or(defaultPosition);
        }

        /**
         * @brief Sets the combat flags for the character.
         * @param flags The new combat flags.
         */
        void SetCombatFlags(const CombatFlags& flags) noexcept
        {
            combatFlags_ = flags;
            EterBase::ModernLogger::Info("Entity {} CombatFlags updated: dead={}, combat={}, stunned={}", 
                                          id_.value(), combatFlags_.isDead, combatFlags_.isInCombat, combatFlags_.isStunned);
            
            DomainCombatFlagsChangeEvent flagsEvent{ id_, combatFlags_ };
            UserInterface::Core::EventBus::GetInstance().Publish(flagsEvent);
        }

        /**
         * @brief Gets the current combat flags of the character.
         * @return The combat flags.
         */
        [[nodiscard]] CombatFlags GetCombatFlags() const noexcept
        {
            return combatFlags_;
        }

    private:
        EterBase::EntityId id_;
        uint32_t hp_{0};
        uint32_t maxHp_{0};
        uint32_t mp_{0};
        uint32_t maxMp_{0};
        uint64_t exp_{0};
        std::optional<GlobalPosition> position_{std::nullopt};
        CombatFlags combatFlags_{};
    };
}
