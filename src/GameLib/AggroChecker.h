#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace CombatMath
{

/**
 * @brief Enum defining actor types to ensure we only check aggro for actual enemies.
 */
enum class ActorType : uint8_t
{
    Enemy = 0,
    NPC = 1,
    Stone = 2,
    Warp = 3,
    Door = 4,
    Building = 5,
    PC = 6,
    Horse = 8,
    Unknown = 255
};

/**
 * @brief Enum defining AI flags. Specifically used to check for AIFLAG_AGGRESSIVE.
 * Matches legacy AIFLAG definitions but strictly typed for C++20.
 */
enum class AIFlag : uint32_t
{
    None = 0,
    Aggressive = 1 << 0, // "AGGR"
    NoMove = 1 << 1,     // "NOMOVE"
    Coward = 1 << 2,     // "COWARD"
    // Other flags omitted for brevity, as we only need Aggressive for this checker.
};

/**
 * @brief Represents the data needed to evaluate a monster's aggro state.
 */
struct MonsterData
{
    uint32_t id;         ///< Unique identifier for the monster instance.
    ActorType type;      ///< The type of the actor (e.g. Enemy, NPC).
    uint32_t aiFlags;    ///< Bitmask representing the AI flags.
};

/**
 * @brief Result structure for a single aggro check.
 */
struct AggroCheckResult
{
    uint32_t id;         ///< Identifier of the checked monster.
    bool isAggressive;   ///< True if the monster has the Aggressive AI flag.
};

/**
 * @brief The AggroChecker class is responsible for identifying if a given monster is aggressive.
 * It is completely decoupled from the GUI and updates only memory state.
 */
class AggroChecker
{
public:
    /**
     * @brief Checks if a single monster is aggressive based on its data.
     * 
     * @param data The data of the monster to check.
     * @return std::optional<AggroCheckResult> Result of the check, or std::nullopt if the actor is not an enemy.
     */
    static std::optional<AggroCheckResult> CheckIsAggressive(const MonsterData& data)
    {
        if (data.type != ActorType::Enemy)
        {
            return std::nullopt;
        }

        bool isAggr = (data.aiFlags & static_cast<uint32_t>(AIFlag::Aggressive)) != 0;
        return AggroCheckResult{data.id, isAggr};
    }

    /**
     * @brief Checks a batch of monsters to identify which ones are aggressive.
     * 
     * @param monsters A span containing the data of multiple monsters.
     * @return std::vector<AggroCheckResult> A list of successful checks.
     */
    static std::vector<AggroCheckResult> CheckBatch(std::span<const MonsterData> monsters)
    {
        std::vector<AggroCheckResult> results;
        results.reserve(monsters.size());

        for (const auto& monster : monsters)
        {
            auto result = CheckIsAggressive(monster);
            if (result.has_value())
            {
                results.push_back(result.value());
            }
        }

        return results;
    }
};

} // namespace CombatMath
