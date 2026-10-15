#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <expected>
#include <format>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents a coordinate in the game world.
 */
struct Coordinates {
    int32_t x;
    int32_t y;
};

/**
 * @brief Represents a single teleportation destination.
 */
struct WarpDestination {
    EterBase::MapIndex mapIndex;          ///< The index of the target map.
    Coordinates coordinates;              ///< The target coordinates.
    EterBase::PlayerLevel requiredLevel;  ///< The minimum level required to warp.
    uint64_t requiredGold;                ///< The cost in gold to warp.
    std::string mapName;                  ///< The display name of the map.
};

/**
 * @brief Event published when the warp destinations for a specific NPC are updated.
 */
struct WarpRegistryUpdatedEvent : public Core::IEvent {
    EterBase::EntityId npcId;

    /**
     * @brief Constructs a new Warp Registry Updated Event object.
     * @param npcId The entity ID of the NPC whose destinations were updated.
     */
    explicit WarpRegistryUpdatedEvent(EterBase::EntityId npcId) : npcId(npcId) {}
};

/**
 * @brief Defines domain errors specific to the warp registry.
 */
enum class WarpRegistryError : uint8_t {
    None = 0,
    NpcNotFound,
    InvalidDestinationIndex,
    InsufficientLevel,
    InsufficientGold
};

/**
 * @brief Converts WarpRegistryError to a string representation for logging.
 * @param err The error to convert.
 * @return A string view representing the error.
 */
[[nodiscard]] constexpr std::string_view ToString(WarpRegistryError err) noexcept {
    switch (err) {
        case WarpRegistryError::None: return "None";
        case WarpRegistryError::NpcNotFound: return "NpcNotFound";
        case WarpRegistryError::InvalidDestinationIndex: return "InvalidDestinationIndex";
        case WarpRegistryError::InsufficientLevel: return "InsufficientLevel";
        case WarpRegistryError::InsufficientGold: return "InsufficientGold";
    }
    return "UnknownWarpRegistryError";
}

/**
 * @brief Result alias for warp registry operations.
 */
template <typename T = void>
using WarpResult = EterBase::Result<T, WarpRegistryError>;

/**
 * @brief Manages available teleportation points provided by NPCs (teleporters).
 * 
 * This model stores the list of warp destinations associated with specific NPC entities.
 * It provides validation methods for warp requests and emits events when the registry is updated.
 */
class WarpRegistryModel {
public:
    /**
     * @brief Retrieves the singleton instance of the WarpRegistryModel.
     * @return Reference to the WarpRegistryModel instance.
     */
    static WarpRegistryModel& GetInstance() {
        static WarpRegistryModel instance;
        return instance;
    }

    /**
     * @brief Adds a new warp destination to a specific NPC.
     * @param npcId The entity ID of the NPC providing the warp.
     * @param destination The destination details.
     * @return Success or error.
     */
    WarpResult<void> AddDestination(EterBase::EntityId npcId, const WarpDestination& destination) {
        destinations_[npcId].push_back(destination);
        EterBase::ModernLogger::Info("Added warp destination '{}' for NPC {}.", destination.mapName, npcId.value());
        
        Core::EventBus::GetInstance().Publish(WarpRegistryUpdatedEvent{npcId});
        return {};
    }

    /**
     * @brief Clears all destinations for a specific NPC.
     * @param npcId The entity ID of the NPC.
     * @return Success or error if the NPC was not found.
     */
    WarpResult<void> ClearDestinations(EterBase::EntityId npcId) {
        if (!destinations_.contains(npcId)) {
            EterBase::ModernLogger::Warn("Attempted to clear destinations for unknown NPC {}.", npcId.value());
            return std::unexpected(WarpRegistryError::NpcNotFound);
        }

        destinations_.erase(npcId);
        EterBase::ModernLogger::Info("Cleared warp destinations for NPC {}.", npcId.value());
        
        Core::EventBus::GetInstance().Publish(WarpRegistryUpdatedEvent{npcId});
        return {};
    }

    /**
     * @brief Retrieves all destinations for a specific NPC.
     * @param npcId The entity ID of the NPC.
     * @return A list of destinations or an error if the NPC was not found.
     */
    WarpResult<std::vector<WarpDestination>> GetDestinations(EterBase::EntityId npcId) const {
        if (auto it = destinations_.find(npcId); it != destinations_.end()) {
            return it->second;
        }
        return std::unexpected(WarpRegistryError::NpcNotFound);
    }

    /**
     * @brief Validates if a player can warp to a specific destination.
     * @param npcId The entity ID of the NPC.
     * @param destinationIndex The index of the destination in the NPC's list.
     * @param playerLevel The current level of the player.
     * @param playerGold The current gold amount of the player.
     * @return The validated WarpDestination or an error.
     */
    WarpResult<WarpDestination> ValidateWarpRequest(
        EterBase::EntityId npcId, 
        size_t destinationIndex, 
        EterBase::PlayerLevel playerLevel, 
        uint64_t playerGold) const 
    {
        return GetDestinations(npcId)
            .and_then([destinationIndex](const std::vector<WarpDestination>& dests) -> WarpResult<WarpDestination> {
                if (destinationIndex >= dests.size()) {
                    return std::unexpected(WarpRegistryError::InvalidDestinationIndex);
                }
                return dests[destinationIndex];
            })
            .and_then([playerLevel](const WarpDestination& dest) -> WarpResult<WarpDestination> {
                if (playerLevel.value() < dest.requiredLevel.value()) {
                    return std::unexpected(WarpRegistryError::InsufficientLevel);
                }
                return dest;
            })
            .and_then([playerGold](const WarpDestination& dest) -> WarpResult<WarpDestination> {
                if (playerGold < dest.requiredGold) {
                    return std::unexpected(WarpRegistryError::InsufficientGold);
                }
                return dest;
            });
    }

private:
    WarpRegistryModel() = default;
    ~WarpRegistryModel() = default;
    
    WarpRegistryModel(const WarpRegistryModel&) = delete;
    WarpRegistryModel& operator=(const WarpRegistryModel&) = delete;
    WarpRegistryModel(WarpRegistryModel&&) = delete;
    WarpRegistryModel& operator=(WarpRegistryModel&&) = delete;

    std::unordered_map<EterBase::EntityId, std::vector<WarpDestination>> destinations_;
};

} // namespace UserInterface::Domain

template <>
struct std::formatter<UserInterface::Domain::WarpRegistryError> : std::formatter<std::string_view> {
    auto format(UserInterface::Domain::WarpRegistryError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(UserInterface::Domain::ToString(err), ctx);
    }
};
