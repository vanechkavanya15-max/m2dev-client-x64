#pragma once

#include <cstdint>
#include <optional>
#include <expected>
#include <algorithm>
#include <string_view>

#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "Core/EventBus.h" 

namespace Metin2::CombatMath {

/**
 * @brief Event emitted when a drop rate probability has been calculated.
 */
struct DropRateCalculatedEvent : public Core::IEvent {
    EterBase::EntityId attackerId;
    EterBase::EntityId monsterId;
    EterBase::ItemVnum itemVnum;
    uint32_t finalProbability;

    /**
     * @brief Constructs a new DropRateCalculatedEvent.
     * @param attacker The attacker's entity ID.
     * @param monster The monster's entity ID.
     * @param item The item's VNUM.
     * @param probability The final calculated probability.
     */
    DropRateCalculatedEvent(EterBase::EntityId attacker, EterBase::EntityId monster, EterBase::ItemVnum item, uint32_t probability)
        : attackerId(attacker), monsterId(monster), itemVnum(item), finalProbability(probability) {}
};

/**
 * @brief Calculator for item drop probabilities.
 */
class DropRateProbabilityCalculator {
public:
    /**
     * @brief Calculates the probability of an item dropping.
     * 
     * @param attackerLevel The level of the attacker.
     * @param monsterLevel The level of the monster.
     * @param baseDropRate The base drop rate of the item.
     * @return std::expected<uint32_t, std::string_view> The calculated probability, or an error message.
     */
    static std::expected<uint32_t, std::string_view> CalculateDropProbability(
        EterBase::PlayerLevel attackerLevel, 
        EterBase::PlayerLevel monsterLevel, 
        uint32_t baseDropRate) noexcept 
    {
        if (attackerLevel.value() == 0 || monsterLevel.value() == 0) {
            return std::unexpected("Invalid level provided (0).");
        }

        std::optional<uint32_t> penalty = std::nullopt;

        // Calculate penalty based on level difference
        if (attackerLevel.value() > monsterLevel.value()) {
            uint8_t diff = attackerLevel.value() - monsterLevel.value();
            if (diff > 10) {
                penalty = 50; // 50% penalty
            } else if (diff > 5) {
                penalty = 25; // 25% penalty
            }
        }

        uint32_t finalRate = baseDropRate;

        // Monadic optional use
        finalRate = penalty.transform([&](uint32_t p) {
            return finalRate * (100 - p) / 100;
        }).value_or(finalRate);

        return finalRate;
    }

    /**
     * @brief Calculates the drop probability and publishes an event.
     * 
     * @param attackerId The entity ID of the attacker.
     * @param monsterId The entity ID of the monster.
     * @param itemVnum The VNUM of the item.
     * @param attackerLevel The level of the attacker.
     * @param monsterLevel The level of the monster.
     * @param baseDropRate The base drop rate of the item.
     */
    static void CalculateAndPublish(
        EterBase::EntityId attackerId,
        EterBase::EntityId monsterId,
        EterBase::ItemVnum itemVnum,
        EterBase::PlayerLevel attackerLevel,
        EterBase::PlayerLevel monsterLevel,
        uint32_t baseDropRate) 
    {
        auto result = CalculateDropProbability(attackerLevel, monsterLevel, baseDropRate);
        
        if (result.has_value()) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Calculated drop rate for item {} from monster {}: {}", itemVnum.value(), monsterId.value(), result.value());
            
            DropRateCalculatedEvent event(attackerId, monsterId, itemVnum, result.value());
            Core::EventBus::Instance().Publish(event);
        } else {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "Failed to calculate drop rate: {}", result.error());
        }
    }
};

} // namespace Metin2::CombatMath
