#include "TextTailEngine.h"
#include "../../EterBase/ModernLogger.h"
#include <cmath>
#include <algorithm>

namespace UserInterface::TextTail {

    EterBase::PacketResult<void> TextTailEngine::RegisterActor(EterBase::EntityId vid, const std::string& name, float modelHeight) {
        if (m_textTails.contains(vid)) {
            EterBase::ModernLogger::Error("Actor {} already registered in TextTailEngine", vid.Get());
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader); 
        }

        ActorTextTail tail;
        tail.vid = vid;
        tail.name = name;
        tail.modelHeight = modelHeight;
        
        // Mocking some bounding box dimensions based on text length
        float assumedWidth = static_cast<float>(name.length()) * 8.0f; 
        tail.nameSize = {assumedWidth, 16.0f};

        m_textTails[vid] = tail;
        m_sortedActorIds.push_back(vid);

        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::UnregisterActor(EterBase::EntityId vid) {
        if (!m_textTails.contains(vid)) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader); // Not found
        }
        
        m_textTails.erase(vid);
        std::erase(m_sortedActorIds, vid);
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::SetActorGuild(EterBase::EntityId vid, const std::string& guildName) {
        auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        it->second.guildName = guildName;
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::SetActorTitle(EterBase::EntityId vid, const std::string& title) {
        auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        it->second.title = title;
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::SetActorKarma(EterBase::EntityId vid, AlignmentKarma karma) {
         auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        it->second.karma = karma;
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::ShowHpBar(EterBase::EntityId vid, float initialHpPercentage) {
        auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        it->second.hasHpBar = true;
        it->second.hpPercentage = std::clamp(initialHpPercentage, 0.0f, 1.0f);
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::UpdateHpBar(EterBase::EntityId vid, float hpPercentage) {
        auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        if (it->second.hasHpBar) {
             it->second.hpPercentage = std::clamp(hpPercentage, 0.0f, 1.0f);
        }
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::HideHpBar(EterBase::EntityId vid) {
         auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        it->second.hasHpBar = false;
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::RegisterItem(uint32_t virtualId, const std::string& name, const Vector3& worldPos) {
        if (m_itemTails.contains(virtualId)) {
             return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        ItemTextTail tail;
        tail.virtualId = virtualId;
        tail.itemName = name;
        tail.worldPos = worldPos;
        m_itemTails[virtualId] = tail;
        m_sortedItemIds.push_back(virtualId);
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::UnregisterItem(uint32_t virtualId) {
        if (!m_itemTails.contains(virtualId)) {
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        m_itemTails.erase(virtualId);
        std::erase(m_sortedItemIds, virtualId);
        return {};
    }

    std::optional<Vector2> TextTailEngine::ProjectWorldToScreen(const Vector3& worldPos, const CameraState& camera) const {
        float viewX = worldPos.x * camera.viewMatrix[0] + worldPos.y * camera.viewMatrix[4] + worldPos.z * camera.viewMatrix[8] + camera.viewMatrix[12];
        float viewY = worldPos.x * camera.viewMatrix[1] + worldPos.y * camera.viewMatrix[5] + worldPos.z * camera.viewMatrix[9] + camera.viewMatrix[13];
        float viewZ = worldPos.x * camera.viewMatrix[2] + worldPos.y * camera.viewMatrix[6] + worldPos.z * camera.viewMatrix[10] + camera.viewMatrix[14];
        float viewW = worldPos.x * camera.viewMatrix[3] + worldPos.y * camera.viewMatrix[7] + worldPos.z * camera.viewMatrix[11] + camera.viewMatrix[15];

        float projX = viewX * camera.projMatrix[0] + viewY * camera.projMatrix[4] + viewZ * camera.projMatrix[8] + viewW * camera.projMatrix[12];
        float projY = viewX * camera.projMatrix[1] + viewY * camera.projMatrix[5] + viewZ * camera.projMatrix[9] + viewW * camera.projMatrix[13];
        float projZ = viewX * camera.projMatrix[2] + viewY * camera.projMatrix[6] + viewZ * camera.projMatrix[10] + viewW * camera.projMatrix[14];
        float projW = viewX * camera.projMatrix[3] + viewY * camera.projMatrix[7] + viewZ * camera.projMatrix[11] + viewW * camera.projMatrix[15];

        if (projW == 0.0f) return std::nullopt;

        projX /= projW;
        projY /= projW;
        projZ /= projW;

        if (projZ < 0.0f || projZ > 1.0f) return std::nullopt;

        float screenX = (projX + 1.0f) * 0.5f * camera.screenWidth;
        float screenY = (1.0f - projY) * 0.5f * camera.screenHeight;

        return Vector2{screenX, screenY};
    }

    EterBase::PacketResult<void> TextTailEngine::UpdateProjection(EterBase::EntityId vid, const Vector3& worldPos, const CameraState& camera) {
        auto it = m_textTails.find(vid);
        if (it == m_textTails.end()) return EterBase::MakeError(EterBase::PacketError::InvalidHeader);

        // Distance from camera to world position
        float dx = worldPos.x - camera.position.x;
        float dy = worldPos.y - camera.position.y;
        float dz = worldPos.z - camera.position.z;
        it->second.distance = std::sqrt(dx*dx + dy*dy + dz*dz);

        Vector3 targetPos = worldPos;
        targetPos.z += it->second.modelHeight; // Render above model

        auto projected = ProjectWorldToScreen(targetPos, camera);
        if (projected) {
            it->second.screenPos = projected.value();
            it->second.isVisible = true;

            // Compute bounds for collision detection
            float halfWidth = it->second.nameSize.x / 2.0f;
            float halfHeight = it->second.nameSize.y / 2.0f;
            it->second.screenBounds.minPos = {it->second.screenPos.x - halfWidth, it->second.screenPos.y - halfHeight};
            it->second.screenBounds.maxPos = {it->second.screenPos.x + halfWidth, it->second.screenPos.y + halfHeight};
            
        } else {
            it->second.isVisible = false;
        }
        return {};
    }

    EterBase::PacketResult<void> TextTailEngine::UpdateItemProjection(uint32_t virtualId, const CameraState& camera) {
        auto it = m_itemTails.find(virtualId);
        if (it == m_itemTails.end()) return EterBase::MakeError(EterBase::PacketError::InvalidHeader);

        float dx = it->second.worldPos.x - camera.position.x;
        float dy = it->second.worldPos.y - camera.position.y;
        float dz = it->second.worldPos.z - camera.position.z;
        it->second.distance = std::sqrt(dx*dx + dy*dy + dz*dz);

        auto projected = ProjectWorldToScreen(it->second.worldPos, camera);
        if (projected) {
            it->second.screenPos = projected.value();
            it->second.isVisible = true;

            float approxWidth = static_cast<float>(it->second.itemName.length()) * 8.0f;
            it->second.screenBounds.minPos = {it->second.screenPos.x - approxWidth/2, it->second.screenPos.y - 8.0f};
            it->second.screenBounds.maxPos = {it->second.screenPos.x + approxWidth/2, it->second.screenPos.y + 8.0f};
        } else {
            it->second.isVisible = false;
        }
        return {};
    }

    void TextTailEngine::UpdateAllProjections(const std::unordered_map<EterBase::EntityId, Vector3>& worldPositions, const CameraState& camera) {
        for (auto& [vid, tail] : m_textTails) {
            auto posIt = worldPositions.find(vid);
            if (posIt != worldPositions.end()) {
                UpdateProjection(vid, posIt->second, camera);
            } else {
                tail.isVisible = false;
            }
        }

        for (auto& [vid, tail] : m_itemTails) {
            UpdateItemProjection(vid, camera);
        }

        SortTailsByDistance();
        DetectAndResolveCollisions();
    }

    void TextTailEngine::SortTailsByDistance() {
        // Sort Actors (back to front)
        std::sort(m_sortedActorIds.begin(), m_sortedActorIds.end(), [this](EterBase::EntityId a, EterBase::EntityId b) {
            return m_textTails[a].distance > m_textTails[b].distance;
        });

        // Sort Items
        std::sort(m_sortedItemIds.begin(), m_sortedItemIds.end(), [this](uint32_t a, uint32_t b) {
            return m_itemTails[a].distance > m_itemTails[b].distance;
        });
    }

    bool TextTailEngine::RectIntersect(const BoundingBox& a, const BoundingBox& b) const {
        return !(a.maxPos.x < b.minPos.x || a.minPos.x > b.maxPos.x ||
                 a.maxPos.y < b.minPos.y || a.minPos.y > b.maxPos.y);
    }

    void TextTailEngine::DetectAndResolveCollisions() {
        // Highly simplified O(N^2) pushing logic as mentioned in CLIENT_SYSTEM_MAP docs
        // Real logic would be more robust. Here we just bump Y up if overlapping.
        for (size_t i = 0; i < m_sortedItemIds.size(); ++i) {
            auto& a = m_itemTails[m_sortedItemIds[i]];
            if (!a.isVisible) continue;

            for (size_t j = 0; j < i; ++j) {
                 auto& b = m_itemTails[m_sortedItemIds[j]];
                 if (!b.isVisible) continue;

                 if (RectIntersect(a.screenBounds, b.screenBounds)) {
                     // push 'a' up
                     float overlap = b.screenBounds.minPos.y - a.screenBounds.maxPos.y;
                     a.screenPos.y += overlap - 2.0f; // push up with padding

                     // update bounds
                     float approxWidth = static_cast<float>(a.itemName.length()) * 8.0f;
                     a.screenBounds.minPos = {a.screenPos.x - approxWidth/2, a.screenPos.y - 8.0f};
                     a.screenBounds.maxPos = {a.screenPos.x + approxWidth/2, a.screenPos.y + 8.0f};
                 }
            }
        }
    }

    std::optional<ActorTextTail> TextTailEngine::GetActorTextTail(EterBase::EntityId vid) const {
        auto it = m_textTails.find(vid);
        if (it != m_textTails.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<ItemTextTail> TextTailEngine::GetItemTextTail(uint32_t virtualId) const {
        auto it = m_itemTails.find(virtualId);
        if (it != m_itemTails.end()) return it->second;
        return std::nullopt;
    }

    const std::unordered_map<EterBase::EntityId, ActorTextTail>& TextTailEngine::GetAllTextTails() const {
        return m_textTails;
    }

    const std::unordered_map<uint32_t, ItemTextTail>& TextTailEngine::GetAllItemTails() const {
        return m_itemTails;
    }

} // namespace UserInterface::TextTail
