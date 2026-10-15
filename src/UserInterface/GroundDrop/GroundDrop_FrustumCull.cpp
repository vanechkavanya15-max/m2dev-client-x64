#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../EterLib/GrpScreen.h"
#include "../../SphereLib/frustum.h"
#include "../../SphereLib/vector.h"

#include <unordered_map>
#include <vector>
#include <cmath>
#include <format>
#include <string_view>

namespace UserInterface::GroundDrop {

    /**
     * @brief C++23 Zero-Conflict Implementation of IGroundDropBatchRenderer for Frustum Culling
     * 
     * This class uses CScreen::GetFrustum() to perform frustum culling on ground drop items.
     * It strictly adheres to Single Responsibility Principle, and decoupled zero-conflict design.
     */
    class GroundDropFrustumCull final : public IGroundDropBatchRenderer {
    public:
        GroundDropFrustumCull() {
            EterBase::ModernLogger::Info("GroundDropFrustumCull: Initialized.");
        }

        ~GroundDropFrustumCull() override {
            EterBase::ModernLogger::Info("GroundDropFrustumCull: Destroyed.");
        }

        /**
         * @brief Adds a new drop to the registry.
         * @param item The ground drop item data.
         * @return EterBase::PacketResult<void> Returns success or error if item already exists.
         */
        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override {
            if (drops_.contains(item.virtualId)) {
                EterBase::ModernLogger::Warning("GroundDropFrustumCull: Attempted to add already existing drop (virtualId: {})", item.virtualId);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            drops_[item.virtualId] = item;
            EterBase::ModernLogger::Debug("GroundDropFrustumCull: Added drop (virtualId: {}, itemVnum: {})", 
                item.virtualId, item.itemVnum.value());
            
            return {};
        }

        /**
         * @brief Removes a drop from the registry.
         * @param virtualId The virtual ID of the drop to remove.
         * @return EterBase::PacketResult<void> Returns success or error if item does not exist.
         */
        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override {
            if (drops_.erase(virtualId) == 0) {
                EterBase::ModernLogger::Warning("GroundDropFrustumCull: Attempted to remove non-existent drop (virtualId: {})", virtualId);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::ModernLogger::Debug("GroundDropFrustumCull: Removed drop (virtualId: {})", virtualId);
            return {};
        }

        /**
         * @brief Updates visibility of drops based on the current camera frustum.
         * @param deltaTime The time elapsed since the last update.
         */
        void UpdateDrops(float /*deltaTime*/) override {
            visibleDrops_.clear();

            // Get global view frustum for culling
            const Frustum& frustum = CScreen::GetFrustum();

            // Standard drop radius estimation for culling 
            // Ensures even drops near the edge of the screen are visible
            constexpr float DROP_RADIUS = 50.0f;

            for (const auto& [virtualId, item] : drops_) {
                Vector3d center(item.x, item.y, item.z);
                
                ViewState state = frustum.ViewVolumeTest(center, DROP_RADIUS);
                if (state == VS_INSIDE || state == VS_PARTIAL) {
                    visibleDrops_.push_back(virtualId);
                }
            }

            EterBase::ModernLogger::Trace("GroundDropFrustumCull: Frustum update finished. Visible: {} / Total: {}", 
                visibleDrops_.size(), drops_.size());
        }

        /**
         * @brief Mocked instanced rendering function. 
         * Real instanced rendering would iterate visibleDrops_.
         */
        void RenderInstancedDrops() override {
            if (!visibleDrops_.empty()) {
                EterBase::ModernLogger::Trace("GroundDropFrustumCull: Rendering {} instanced drops.", visibleDrops_.size());
            }
        }

        /**
         * @brief Checks if a player is in range to pick up a specific drop.
         * @param virtualId Drop ID to check.
         * @param playerX Player X coordinate.
         * @param playerY Player Y coordinate.
         * @param maxRange Maximum allowed distance for picking.
         * @return true if in range, false otherwise.
         */
        bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const override {
            auto it = drops_.find(virtualId);
            if (it == drops_.end()) {
                return false;
            }

            const GroundDropItemData& item = it->second;
            float dx = item.x - playerX;
            float dy = item.y - playerY;
            
            return (dx * dx + dy * dy) <= (maxRange * maxRange);
        }

        /**
         * @brief Clears all tracked drops.
         */
        void ClearAll() override {
            drops_.clear();
            visibleDrops_.clear();
            EterBase::ModernLogger::Info("GroundDropFrustumCull: Cleared all drops.");
        }

    private:
        std::unordered_map<uint32_t, GroundDropItemData> drops_;
        std::vector<uint32_t> visibleDrops_;
    };

} // namespace UserInterface::GroundDrop
