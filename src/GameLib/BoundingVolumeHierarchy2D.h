#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <format>
#include <limits>
#include "../EterBase/Result.h"
#include "../EterBase/StrongTypes.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Represents an Axis-Aligned Bounding Box (AABB) in 2D space.
 */
struct AABB2D {
    float minX;
    float minY;
    float maxX;
    float maxY;

    /**
     * @brief Checks if this AABB intersects with another AABB.
     * @param other The other AABB to test against.
     * @return true if there is an intersection, false otherwise.
     */
    [[nodiscard]] constexpr bool Intersects(const AABB2D& other) const noexcept {
        return minX <= other.maxX && maxX >= other.minX &&
               minY <= other.maxY && maxY >= other.minY;
    }

    /**
     * @brief Merges this AABB with another AABB.
     * @param other The other AABB to merge with.
     * @return A new AABB that contains both this and the other AABB.
     */
    [[nodiscard]] constexpr AABB2D Merge(const AABB2D& other) const noexcept {
        return {
            std::min(minX, other.minX),
            std::min(minY, other.minY),
            std::max(maxX, other.maxX),
            std::max(maxY, other.maxY)
        };
    }
};

/**
 * @brief Represents a single entity (wall/obstacle) with its bounding box in the BVH.
 */
struct BVHEntity {
    EterBase::EntityId id;
    AABB2D bounds;
};

/**
 * @brief Event published when a collision is detected in the BVH.
 */
struct BVHCollisionEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId sourceId;
    EterBase::EntityId hitId;

    /**
     * @brief Constructs a new BVH Collision Event.
     * @param sourceId The entity initiating the collision check.
     * @param hitId The entity that was hit.
     */
    BVHCollisionEvent(EterBase::EntityId sourceId, EterBase::EntityId hitId)
        : sourceId(sourceId), hitId(hitId) {}
};

/**
 * @brief A Bounding Volume Hierarchy (BVH) for 2D space to accelerate collision detection.
 */
class BoundingVolumeHierarchy2D {
public:
    /**
     * @brief Constructs an empty BVH.
     */
    BoundingVolumeHierarchy2D() = default;

    /**
     * @brief Builds the BVH from a list of entities.
     * @param entities The entities to insert into the BVH.
     * @return EterBase::VoidResult indicating success or failure.
     */
    EterBase::VoidResult<EterBase::NavigationError> Build(std::vector<BVHEntity> entities) {
        if (entities.empty()) {
            EterBase::ModernLogger::Warn("BoundingVolumeHierarchy2D::Build - Empty entity list.");
            return EterBase::MakeError(EterBase::NavigationError::MapNotLoaded);
        }

        m_nodes.clear();
        m_nodes.reserve(entities.size() * 2);

        m_rootIndex = BuildRecursive(entities, 0, entities.size());
        
        if (m_rootIndex) {
            EterBase::ModernLogger::Info("BoundingVolumeHierarchy2D::Build - BVH successfully built with {} nodes.", m_nodes.size());
            return {};
        }

        EterBase::ModernLogger::Error("BoundingVolumeHierarchy2D::Build - Failed to build BVH.");
        return EterBase::MakeError(EterBase::NavigationError::BlockedTerrain);
    }

    /**
     * @brief Queries the BVH for the first entity that intersects with the given bounds.
     * @param sourceId The ID of the entity making the query (for event publishing).
     * @param queryBounds The bounding box to test for collisions.
     * @return std::optional<EterBase::EntityId> containing the ID of the hit entity, or std::nullopt.
     */
    [[nodiscard]] std::optional<EterBase::EntityId> QueryFirst(EterBase::EntityId sourceId, const AABB2D& queryBounds) const {
        if (!m_rootIndex) {
            return std::nullopt;
        }

        return QueryRecursive(*m_rootIndex, queryBounds)
            .and_then([sourceId](EterBase::EntityId hitId) -> std::optional<EterBase::EntityId> {
                // Publish event on collision
                UserInterface::Core::EventBus::GetInstance().Publish(BVHCollisionEvent(sourceId, hitId));
                return hitId;
            });
    }

private:
    struct BVHNode {
        AABB2D bounds;
        std::optional<EterBase::EntityId> entityId;
        std::optional<size_t> leftChild;
        std::optional<size_t> rightChild;
        
        [[nodiscard]] bool IsLeaf() const noexcept {
            return entityId.has_value();
        }
    };

    std::vector<BVHNode> m_nodes;
    std::optional<size_t> m_rootIndex;

    /**
     * @brief Recursively builds the BVH tree.
     * @param entities The full list of entities.
     * @param start The starting index for the current node.
     * @param end The ending index (exclusive) for the current node.
     * @return std::optional<size_t> representing the index of the created node in m_nodes.
     */
    std::optional<size_t> BuildRecursive(std::vector<BVHEntity>& entities, size_t start, size_t end) {
        if (start >= end) {
            return std::nullopt;
        }

        size_t nodeIndex = m_nodes.size();
        m_nodes.push_back(BVHNode{});
        
        if (end - start == 1) {
            m_nodes[nodeIndex].bounds = entities[start].bounds;
            m_nodes[nodeIndex].entityId = entities[start].id;
            return nodeIndex;
        }

        // Calculate bounding box of centroids
        AABB2D centroidBounds = {
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()
        };

        for (size_t i = start; i < end; ++i) {
            float cx = (entities[i].bounds.minX + entities[i].bounds.maxX) * 0.5f;
            float cy = (entities[i].bounds.minY + entities[i].bounds.maxY) * 0.5f;
            centroidBounds.minX = std::min(centroidBounds.minX, cx);
            centroidBounds.minY = std::min(centroidBounds.minY, cy);
            centroidBounds.maxX = std::max(centroidBounds.maxX, cx);
            centroidBounds.maxY = std::max(centroidBounds.maxY, cy);
        }

        int axis = (centroidBounds.maxX - centroidBounds.minX > centroidBounds.maxY - centroidBounds.minY) ? 0 : 1;

        std::sort(entities.begin() + start, entities.begin() + end, [axis](const BVHEntity& a, const BVHEntity& b) {
            float ca = (axis == 0) ? (a.bounds.minX + a.bounds.maxX) : (a.bounds.minY + a.bounds.maxY);
            float cb = (axis == 0) ? (b.bounds.minX + b.bounds.maxX) : (b.bounds.minY + b.bounds.maxY);
            return ca < cb;
        });

        size_t mid = start + (end - start) / 2;
        
        m_nodes[nodeIndex].leftChild = BuildRecursive(entities, start, mid);
        m_nodes[nodeIndex].rightChild = BuildRecursive(entities, mid, end);

        // Compute bounds based on children
        AABB2D nodeBounds = {
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()
        };

        auto mergeBounds = [&](std::optional<size_t> childIdx) {
            if (childIdx) {
                nodeBounds = nodeBounds.Merge(m_nodes[*childIdx].bounds);
            }
        };

        mergeBounds(m_nodes[nodeIndex].leftChild);
        mergeBounds(m_nodes[nodeIndex].rightChild);
        
        m_nodes[nodeIndex].bounds = nodeBounds;

        return nodeIndex;
    }

    /**
     * @brief Recursively queries the BVH tree.
     * @param nodeIndex The current node index to check.
     * @param queryBounds The bounding box to test for collisions.
     * @return std::optional<EterBase::EntityId> containing the hit entity ID, if any.
     */
    [[nodiscard]] std::optional<EterBase::EntityId> QueryRecursive(size_t nodeIndex, const AABB2D& queryBounds) const {
        if (nodeIndex >= m_nodes.size()) {
            return std::nullopt;
        }

        const auto& node = m_nodes[nodeIndex];

        if (!node.bounds.Intersects(queryBounds)) {
            return std::nullopt;
        }

        if (node.IsLeaf()) {
            return node.entityId;
        }

        auto hitOpt = node.leftChild.and_then([&](size_t leftIdx) {
            return QueryRecursive(leftIdx, queryBounds);
        });

        if (hitOpt) {
            return hitOpt;
        }

        return node.rightChild.and_then([&](size_t rightIdx) {
            return QueryRecursive(rightIdx, queryBounds);
        });
    }
};

} // namespace GameLib
