#pragma once

#include <expected>
#include <vector>
#include <memory>
#include <optional>
#include <format>
#include <cmath>
#include <algorithm>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Represents a 2D bounding box for spatial partitioning.
 */
struct QuadTreeBoundingBox {
    float x;
    float y;
    float width;
    float height;

    /**
     * @brief Checks if a point is within this bounding box.
     * @param pointX X coordinate.
     * @param pointY Y coordinate.
     * @return True if point is inside, false otherwise.
     */
    [[nodiscard]] constexpr bool Contains(float pointX, float pointY) const noexcept {
        return (pointX >= x && pointX <= x + width &&
                pointY >= y && pointY <= y + height);
    }

    /**
     * @brief Checks if this box intersects with another box.
     * @param other The other bounding box.
     * @return True if they intersect.
     */
    [[nodiscard]] constexpr bool Intersects(const QuadTreeBoundingBox& other) const noexcept {
        return !(other.x > x + width ||
                 other.x + other.width < x ||
                 other.y > y + height ||
                 other.y + other.height < y);
    }
};

/**
 * @brief Represents an entity stored in the QuadTree.
 */
struct QuadTreeEntity {
    EterBase::EntityId id;
    float x;
    float y;
};

/**
 * @brief Event published when an entity is updated (moved) in the QuadTree.
 */
struct EntityMovedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId id;
    float newX;
    float newY;
    
    EntityMovedEvent(EterBase::EntityId entityId, float nx, float ny)
        : id(entityId), newX(nx), newY(ny) {}
};

/**
 * @brief Spatial partitioning data structure for fast 2D radius lookups.
 * Standard 2026 (C++23) compliant.
 */
class QuadTreeSpatialPartition {
public:
    static constexpr size_t NODE_CAPACITY = 4;
    static constexpr size_t MAX_DEPTH = 8;

    /**
     * @brief Constructs the root of the QuadTree.
     * @param bounds The full boundary this tree represents.
     */
    explicit QuadTreeSpatialPartition(QuadTreeBoundingBox bounds)
        : m_bounds(bounds), m_depth(0) {}

    ~QuadTreeSpatialPartition() = default;
    QuadTreeSpatialPartition(const QuadTreeSpatialPartition&) = delete;
    QuadTreeSpatialPartition& operator=(const QuadTreeSpatialPartition&) = delete;

    /**
     * @brief Inserts an entity into the QuadTree.
     * @param entity The entity to insert.
     * @return EterBase::Result indicating success or an EntityError.
     */
    EterBase::Result<void, EterBase::EntityError> Insert(QuadTreeEntity entity) {
        if (!m_bounds.Contains(entity.x, entity.y)) {
            return EterBase::MakeError(EterBase::EntityError::OutOfRange);
        }

        if (m_children.empty() && m_entities.size() < NODE_CAPACITY) {
            m_entities.push_back(entity);
            return {};
        }

        if (m_depth < MAX_DEPTH && m_children.empty()) {
            Subdivide();
            
            // Re-distribute existing entities to children
            auto currentEntities = std::move(m_entities);
            m_entities.clear();
            
            for (const auto& existingEntity : currentEntities) {
                for (auto& child : m_children) {
                    if (child->Insert(existingEntity).has_value()) {
                        break;
                    }
                }
            }
        }

        if (!m_children.empty()) {
            for (auto& child : m_children) {
                if (child->Insert(entity).has_value()) {
                    return {};
                }
            }
        }
        
        // If it fits nowhere in children but is within our bounds (fallback)
        m_entities.push_back(entity);
        return {};
    }

    /**
     * @brief Removes an entity from the QuadTree by ID.
     * @param id The unique EntityId of the entity to remove.
     * @return EterBase::Result indicating success or an EntityError.
     */
    EterBase::Result<void, EterBase::EntityError> Remove(EterBase::EntityId id) {
        auto it = std::remove_if(m_entities.begin(), m_entities.end(),
                                 [id](const QuadTreeEntity& ent) { return ent.id == id; });
        
        if (it != m_entities.end()) {
            m_entities.erase(it, m_entities.end());
            return {};
        }

        if (!m_children.empty()) {
            for (auto& child : m_children) {
                if (child->Remove(id).has_value()) {
                    return {};
                }
            }
        }

        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    /**
     * @brief Updates an entity's position within the QuadTree.
     * @param id The unique EntityId of the entity to update.
     * @param newX The new X coordinate.
     * @param newY The new Y coordinate.
     * @return EterBase::Result indicating success or an EntityError.
     */
    EterBase::Result<void, EterBase::EntityError> Update(EterBase::EntityId id, float newX, float newY) {
        // Find existing to save state in case insert fails
        auto oldEntityOpt = FindEntityRecursive(id);
        if (!oldEntityOpt.has_value()) {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        if (!m_bounds.Contains(newX, newY)) {
            return EterBase::MakeError(EterBase::EntityError::OutOfRange);
        }

        auto removeResult = Remove(id);
        if (!removeResult.has_value()) {
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        auto insertResult = Insert(QuadTreeEntity{id, newX, newY});
        if (!insertResult.has_value()) {
            // Rollback on failure
            [[maybe_unused]] auto rollbackResult = Insert(*oldEntityOpt);
            return insertResult;
        }

        // Emit an event via EventBus to notify other systems
        UserInterface::Core::EventBus::GetInstance().Publish(EntityMovedEvent{id, newX, newY});
        return {};
    }

    /**
     * @brief Fast lookup of entities within a given radius.
     * @param x The center X coordinate.
     * @param y The center Y coordinate.
     * @param radius The search radius.
     * @return A vector of EntityId representing found entities.
     */
    [[nodiscard]] std::vector<EterBase::EntityId> SearchInRadius(float x, float y, float radius) const {
        std::vector<EterBase::EntityId> result;
        SearchInRadiusRecursive(x, y, radius, result);
        return result;
    }

private:
    /**
     * @brief Recursive helper for radius search.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Search radius.
     * @param result Result vector.
     */
    void SearchInRadiusRecursive(float x, float y, float radius, std::vector<EterBase::EntityId>& result) const {
        // Build a bounding box for the search circle to quickly cull non-intersecting quadtree nodes
        QuadTreeBoundingBox searchBox{x - radius, y - radius, radius * 2.0f, radius * 2.0f};

        if (!m_bounds.Intersects(searchBox)) {
            return;
        }

        float radiusSq = radius * radius;
        for (const auto& entity : m_entities) {
            float dx = entity.x - x;
            float dy = entity.y - y;
            if ((dx * dx + dy * dy) <= radiusSq) {
                result.push_back(entity.id);
            }
        }

        if (!m_children.empty()) {
            for (const auto& child : m_children) {
                child->SearchInRadiusRecursive(x, y, radius, result);
            }
        }
    }
public:
    /**
     * @brief Internal constructor for creating sub-nodes.
     * @param bounds The boundary for this sub-node.
     * @param depth The depth level of this node.
     */
    QuadTreeSpatialPartition(QuadTreeBoundingBox bounds, size_t depth)
        : m_bounds(bounds), m_depth(depth) {}

private:
    QuadTreeBoundingBox m_bounds;
    size_t m_depth;
    std::vector<QuadTreeEntity> m_entities;
    std::vector<std::unique_ptr<QuadTreeSpatialPartition>> m_children;

    /**
     * @brief Subdivides the current node into four quadrants.
     */
    void Subdivide() {
        if (!m_children.empty()) {
            return;
        }
        
        float subWidth = m_bounds.width / 2.0f;
        float subHeight = m_bounds.height / 2.0f;
        float x = m_bounds.x;
        float y = m_bounds.y;

        m_children.reserve(4);
        m_children.push_back(std::make_unique<QuadTreeSpatialPartition>(
            QuadTreeBoundingBox{x, y, subWidth, subHeight}, m_depth + 1));
        m_children.push_back(std::make_unique<QuadTreeSpatialPartition>(
            QuadTreeBoundingBox{x + subWidth, y, subWidth, subHeight}, m_depth + 1));
        m_children.push_back(std::make_unique<QuadTreeSpatialPartition>(
            QuadTreeBoundingBox{x, y + subHeight, subWidth, subHeight}, m_depth + 1));
        m_children.push_back(std::make_unique<QuadTreeSpatialPartition>(
            QuadTreeBoundingBox{x + subWidth, y + subHeight, subWidth, subHeight}, m_depth + 1));
    }

    /**
     * @brief Monadic helper to find an entity by ID in this node recursively.
     */
    [[nodiscard]] std::optional<QuadTreeEntity> FindEntityRecursive(EterBase::EntityId id) const {
        auto it = std::find_if(m_entities.begin(), m_entities.end(),
                               [id](const QuadTreeEntity& ent) { return ent.id == id; });
        
        if (it != m_entities.end()) {
            return *it;
        }

        for (const auto& child : m_children) {
            if (auto found = child->FindEntityRecursive(id)) {
                return found;
            }
        }
        
        return std::nullopt;
    }
};

} // namespace GameLib
