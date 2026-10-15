#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <format>
#include <vector>
#include <chrono>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "EventBus.h"

namespace UserInterface::Core {

/**
 * @brief Represents the data of an entity exported in the telemetry snapshot.
 */
struct AITelemetryEntityData {
    EterBase::EntityId id;      ///< The unique entity ID.
    uint32_t type;              ///< Type or race of the entity.
    int32_t x;                  ///< X coordinate.
    int32_t y;                  ///< Y coordinate.
    int32_t z;                  ///< Z coordinate.
    std::optional<uint32_t> hp; ///< Current HP of the entity, if known.
};

/**
 * @brief Event emitted when an AI telemetry snapshot is generated.
 * 
 * This decouples the AI logic from the telemetry generation logic,
 * replacing archaic direct callback injections or GUI links.
 */
struct AITelemetrySnapshotReadyEvent : public IEvent {
    std::string jsonPayload; ///< The JSON formatted snapshot of the game state.

    /**
     * @brief Constructs the telemetry snapshot event.
     * @param payload The JSON payload representing the current telemetry.
     */
    explicit AITelemetrySnapshotReadyEvent(std::string payload) : jsonPayload(std::move(payload)) {}
};

/**
 * @brief Handles gathering bot state and exporting it to AI agents in JSON format.
 * 
 * Follows C++23 standards, using std::expected for errors and std::optional for monadic safety.
 * Decoupled from the GUI by publishing AITelemetrySnapshotReadyEvent to the Core::EventBus.
 */
class AITelemetrySnapshot {
public:
    AITelemetrySnapshot() = default;
    ~AITelemetrySnapshot() = default;

    /**
     * @brief Generates a JSON snapshot of the provided entities and player state.
     * 
     * @param playerId The entity ID of the main player/bot.
     * @param mapIndex The index of the current map.
     * @param entities A list of currently visible/tracked entities in the environment.
     * @return EterBase::Result<std::string, EterBase::EntityError> The generated JSON string on success, or an EntityError.
     */
    [[nodiscard]] EterBase::Result<std::string, EterBase::EntityError> GenerateSnapshot(
        EterBase::EntityId playerId,
        EterBase::MapIndex mapIndex,
        const std::vector<AITelemetryEntityData>& entities) const 
    {
        if (!playerId) {
            EterBase::ModernLogger::Error("Failed to generate AI telemetry: Invalid player ID");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        std::string entitiesJson = "[";
        for (size_t i = 0; i < entities.size(); ++i) {
            const auto& ent = entities[i];
            
            // Using monadic std::optional to transform or fallback
            std::string hpStr = ent.hp.transform([](uint32_t val) {
                return std::format("{}", val);
            }).value_or("null");

            entitiesJson += std::format(
                R"({{"id":{},"type":{},"x":{},"y":{},"z":{},"hp":{}}})",
                ent.id.value(), ent.type, ent.x, ent.y, ent.z, hpStr
            );

            if (i < entities.size() - 1) {
                entitiesJson += ",";
            }
        }
        entitiesJson += "]";

        std::string finalJson = std::format(
            R"({{"timestamp":{},"player_id":{},"map_index":{},"entities":{}}})",
            GetUnixTimestamp(),
            playerId.value(),
            mapIndex.value(),
            entitiesJson
        );

        EterBase::ModernLogger::Debug("Generated AI telemetry snapshot successfully.");
        return finalJson;
    }

    /**
     * @brief Generates a snapshot and publishes it via EventBus for external consumers.
     * 
     * @param playerId The entity ID of the main player/bot.
     * @param mapIndex The index of the current map.
     * @param entities A list of currently visible/tracked entities in the environment.
     */
    void PublishSnapshot(
        EterBase::EntityId playerId,
        EterBase::MapIndex mapIndex,
        const std::vector<AITelemetryEntityData>& entities) const
    {
        auto result = GenerateSnapshot(playerId, mapIndex, entities);
        
        if (result.has_value()) {
            EventBus::GetInstance().Publish(AITelemetrySnapshotReadyEvent{std::move(result.value())});
            EterBase::ModernLogger::Info("Published AI telemetry snapshot to EventBus.");
        } else {
            EterBase::ModernLogger::Warn("Skipped publishing AI telemetry: generation failed.");
        }
    }

private:
    /**
     * @brief Helper to get the current Unix timestamp in seconds.
     * @return uint64_t Current Unix timestamp.
     */
    [[nodiscard]] uint64_t GetUnixTimestamp() const {
        const auto now = std::chrono::system_clock::now();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    }
};

} // namespace UserInterface::Core
