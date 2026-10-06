#pragma once

#include <unordered_map>
#include <string>
#include <expected>
#include <optional>
#include <span>
#include <functional>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Domain {

/**
 * @brief Represents the fundamental types of actors in the world.
 * Matches legacy C++ EType classifications but using modern C++23 strong enum.
 */
enum class ActorType : uint8_t {
    Enemy = 0,    ///< Potwory agresywne (TYPE_ENEMY)
    Npc = 1,      ///< Przyjazne postacie, handlarze (TYPE_NPC)
    Stone = 2,    ///< Kamienie Metin (TYPE_STONE)
    Warp = 3,     ///< Warp portals
    Door = 4,     ///< Doors
    Building = 5, ///< Buildings
    Player = 6,   ///< Gracz (TYPE_PC)
    Poly = 7,     ///< Polymorphing
    Horse = 8     ///< Wierzchowiec (TYPE_HORSE)
};

/**
 * @brief Core data model for a single actor entity, completely decoupled from rendering/Granny.
 */
struct ActorData {
    EterBase::EntityId id;
    ActorType type;
    std::string name;
    float positionX;
    float positionY;
    float positionZ;
    float rotationAngle;

    /**
     * @brief Constructs new ActorData.
     * @param id The unique entity ID.
     * @param type The categorical type of the actor.
     * @param name Name string of the actor.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     * @param rotation Rotation angle in degrees.
     */
    ActorData(EterBase::EntityId id, ActorType type, std::string name, float x, float y, float z, float rotation)
        : id(id), type(type), name(std::move(name)), positionX(x), positionY(y), positionZ(z), rotationAngle(rotation) {}
};

/**
 * @brief Event triggered when a new actor is spawned and added to the registry.
 */
struct ActorSpawnedEvent : public Core::IEvent {
    EterBase::EntityId id;
    ActorType type;
    
    explicit ActorSpawnedEvent(EterBase::EntityId id, ActorType type) : id(id), type(type) {}
};

/**
 * @brief Event triggered when an actor is removed (despawned or killed) from the registry.
 */
struct ActorDespawnedEvent : public Core::IEvent {
    EterBase::EntityId id;

    explicit ActorDespawnedEvent(EterBase::EntityId id) : id(id) {}
};

/**
 * @brief Event triggered when an actor moves.
 */
struct ActorMovedEvent : public Core::IEvent {
    EterBase::EntityId id;
    float positionX;
    float positionY;
    float positionZ;
    float rotationAngle;

    ActorMovedEvent(EterBase::EntityId id, float x, float y, float z, float rotation)
        : id(id), positionX(x), positionY(y), positionZ(z), rotationAngle(rotation) {}
};

/**
 * @brief Central registry that manages all actors currently loaded in the world.
 * Strictly adheres to the zero-conflict rule by storing data separately from GUI classes.
 */
class ActorRegistryModel {
public:
    ActorRegistryModel() = default;
    ~ActorRegistryModel() = default;

    // Delete copy and move semantics
    ActorRegistryModel(const ActorRegistryModel&) = delete;
    ActorRegistryModel& operator=(const ActorRegistryModel&) = delete;

    /**
     * @brief Adds a new actor to the registry.
     * @param id The unique EntityId for the actor.
     * @param type The ActorType of the actor.
     * @param name The actor's name.
     * @param x Initial X coordinate.
     * @param y Initial Y coordinate.
     * @param z Initial Z coordinate.
     * @param rotation Initial rotation angle.
     * @return std::expected<void, EterBase::EntityError> Success, or EntityError if an actor with this ID already exists.
     */
    std::expected<void, EterBase::EntityError> AddActor(EterBase::EntityId id, ActorType type, const std::string& name, float x, float y, float z, float rotation) {
        if (actors_.contains(id)) {
            EterBase::ModernLogger::Warn("Failed to add actor: ID {} already exists.", id.value());
            return std::unexpected(EterBase::EntityError::AlreadyExists);
        }

        actors_.emplace(id, ActorData{id, type, name, x, y, z, rotation});
        
        Core::EventBus::GetInstance().Publish(ActorSpawnedEvent{id, type});
        EterBase::ModernLogger::Info("Actor added: ID {}, Name {}, Type {}", id.value(), name, static_cast<uint32_t>(type));
        
        return {};
    }

    /**
     * @brief Removes an actor from the registry.
     * @param id The unique EntityId of the actor to remove.
     * @return std::expected<void, EterBase::EntityError> Success, or EntityError if the actor was not found.
     */
    std::expected<void, EterBase::EntityError> RemoveActor(EterBase::EntityId id) {
        if (actors_.erase(id) == 0) {
            EterBase::ModernLogger::Warn("Failed to remove actor: ID {} not found.", id.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        Core::EventBus::GetInstance().Publish(ActorDespawnedEvent{id});
        EterBase::ModernLogger::Info("Actor removed: ID {}", id.value());
        
        return {};
    }

    /**
     * @brief Retrieves an actor by its EntityId.
     * @param id The unique EntityId to lookup.
     * @return std::optional containing a reference to the ActorData if found, std::nullopt otherwise.
     */
    std::optional<std::reference_wrapper<const ActorData>> GetActor(EterBase::EntityId id) const {
        if (auto it = actors_.find(id); it != actors_.end()) {
            return std::cref(it->second);
        }
        return std::nullopt;
    }
    
    /**
     * @brief Updates the transform (position and rotation) of an actor.
     * @param id The unique EntityId.
     * @param x New X coordinate.
     * @param y New Y coordinate.
     * @param z New Z coordinate.
     * @param rotation New rotation angle.
     * @return std::expected<void, EterBase::EntityError> Success, or EntityError if the actor was not found.
     */
    std::expected<void, EterBase::EntityError> UpdateActorTransform(EterBase::EntityId id, float x, float y, float z, float rotation) {
        auto it = actors_.find(id);
        if (it == actors_.end()) {
            EterBase::ModernLogger::Warn("Failed to update transform: Actor ID {} not found.", id.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }
        
        it->second.positionX = x;
        it->second.positionY = y;
        it->second.positionZ = z;
        it->second.rotationAngle = rotation;
        
        Core::EventBus::GetInstance().Publish(ActorMovedEvent{id, x, y, z, rotation});
        
        return {};
    }
    
    /**
     * @brief Gets the total count of registered actors.
     * @return The number of actors.
     */
    [[nodiscard]] size_t GetActorCount() const noexcept {
        return actors_.size();
    }
    
    /**
     * @brief Clears all actors from the registry.
     */
    void Clear() {
        actors_.clear();
        EterBase::ModernLogger::Info("Actor registry cleared.");
    }

private:
    std::unordered_map<EterBase::EntityId, ActorData> actors_;
};

} // namespace UserInterface::Domain
