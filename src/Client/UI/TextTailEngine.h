#pragma once

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <stdexcept>

namespace UserInterface::TextTail {

    // Extended 3D and 2D math structures for projections
    struct Vector3 {
        float x{0.0f}, y{0.0f}, z{0.0f};
    };

    struct Vector2 {
        float x{0.0f}, y{0.0f};
    };

    struct BoundingBox {
        Vector2 minPos;
        Vector2 maxPos;
    };

    enum class AlignmentKarma {
        Cruel,
        Neutral,
        Chivalric
    };

    struct ColorARGB {
        uint32_t argb{0xFFFFFFFF};
    };

    // Advanced Text Tail state representing an actor
    struct ActorTextTail {
        EterBase::EntityId vid{0};
        std::string name;
        std::optional<std::string> guildName;
        std::optional<std::string> title;
        AlignmentKarma karma{AlignmentKarma::Neutral};
        
        float modelHeight{0.0f};
        
        // Projected coordinates and geometry
        Vector2 screenPos;
        Vector2 nameSize{0.0f, 0.0f};
        BoundingBox screenBounds;
        float distance{0.0f};
        bool isVisible{false};
        bool isOccluded{false};

        // HP Bar
        bool hasHpBar{false};
        float hpPercentage{0.0f}; // 0.0 to 1.0
        
        // Fade & Rendering
        float alpha{1.0f};
        uint64_t lastUpdateTime{0};
    };

    // Item tail representation
    struct ItemTextTail {
        uint32_t virtualId{0};
        std::string itemName;
        Vector3 worldPos;
        Vector2 screenPos;
        float distance{0.0f};
        bool isVisible{false};
        float alpha{1.0f};
        BoundingBox screenBounds;
    };

    // Matrices and viewport for Screen Projection
    struct CameraState {
        float viewMatrix[16];
        float projMatrix[16];
        int screenWidth{800};
        int screenHeight{600};
        Vector3 position{0.0f, 0.0f, 0.0f}; // Actual camera world position
    };

    class TextTailEngine {
    public:
        TextTailEngine() = default;
        ~TextTailEngine() = default;

        TextTailEngine(const TextTailEngine&) = delete;
        TextTailEngine& operator=(const TextTailEngine&) = delete;

        // --- ACTOR REGISTRATION ---
        EterBase::PacketResult<void> RegisterActor(EterBase::EntityId vid, const std::string& name, float modelHeight);
        EterBase::PacketResult<void> UnregisterActor(EterBase::EntityId vid);

        // --- PROPERTIES ---
        EterBase::PacketResult<void> SetActorGuild(EterBase::EntityId vid, const std::string& guildName);
        EterBase::PacketResult<void> SetActorTitle(EterBase::EntityId vid, const std::string& title);
        EterBase::PacketResult<void> SetActorKarma(EterBase::EntityId vid, AlignmentKarma karma);

        // --- HP BARS ---
        EterBase::PacketResult<void> ShowHpBar(EterBase::EntityId vid, float initialHpPercentage);
        EterBase::PacketResult<void> UpdateHpBar(EterBase::EntityId vid, float hpPercentage);
        EterBase::PacketResult<void> HideHpBar(EterBase::EntityId vid);

        // --- ITEM REGISTRATION ---
        EterBase::PacketResult<void> RegisterItem(uint32_t virtualId, const std::string& name, const Vector3& worldPos);
        EterBase::PacketResult<void> UnregisterItem(uint32_t virtualId);

        // --- PROJECTION LOGIC ---
        EterBase::PacketResult<void> UpdateProjection(EterBase::EntityId vid, const Vector3& worldPos, const CameraState& camera);
        EterBase::PacketResult<void> UpdateItemProjection(uint32_t virtualId, const CameraState& camera);
        
        // --- BATCH UPDATES ---
        void UpdateAllProjections(const std::unordered_map<EterBase::EntityId, Vector3>& worldPositions, const CameraState& camera);
        
        // --- OCCLUSION AND SORTING ---
        void SortTailsByDistance();
        void DetectAndResolveCollisions(); // Simplistic 2D collision resolution pushing tails up

        // --- ACCESSORS ---
        std::optional<ActorTextTail> GetActorTextTail(EterBase::EntityId vid) const;
        std::optional<ItemTextTail> GetItemTextTail(uint32_t virtualId) const;
        const std::unordered_map<EterBase::EntityId, ActorTextTail>& GetAllTextTails() const;
        const std::unordered_map<uint32_t, ItemTextTail>& GetAllItemTails() const;

    private:
        std::unordered_map<EterBase::EntityId, ActorTextTail> m_textTails;
        std::unordered_map<uint32_t, ItemTextTail> m_itemTails;

        // Sorted lists for rendering
        std::vector<EterBase::EntityId> m_sortedActorIds;
        std::vector<uint32_t> m_sortedItemIds;

        std::optional<Vector2> ProjectWorldToScreen(const Vector3& worldPos, const CameraState& camera) const;
        bool RectIntersect(const BoundingBox& a, const BoundingBox& b) const;
    };

} // namespace UserInterface::TextTail
