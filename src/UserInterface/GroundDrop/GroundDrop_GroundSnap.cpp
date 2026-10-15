#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "../PythonBackground.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include "GroundDropEvents.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief A renderer for ground drops that snaps items to the terrain height.
     */
    class GroundDropBatchRenderer_GroundSnap final : public IGroundDropBatchRenderer
    {
    public:
        GroundDropBatchRenderer_GroundSnap() = default;
        ~GroundDropBatchRenderer_GroundSnap() override = default;

        /**
         * @brief Adds a new drop item, adjusting its Z coordinate based on the map's terrain height.
         * @param item The item data to add.
         * @return EterBase::PacketResult<void> Success or error.
         */
        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override
        {
            auto snappedItem = item;
            
            // Dopasowanie wysokosci lezenia przedmiotu do uksztaltowania terenu (Ground Snap)
            snappedItem.z = CPythonBackground::Instance().GetHeight(snappedItem.x, snappedItem.y);

            EterBase::EntityId entityId{snappedItem.virtualId};

            EterBase::ModernLogger::Info("GroundDropBatchRenderer_GroundSnap: Added drop VID {} at x: {}, y: {}, z snapped to: {}",
                                         entityId.get(), snappedItem.x, snappedItem.y, snappedItem.z);

            m_drops.push_back(snappedItem);

            UserInterface::Core::EventBus::GetInstance().Publish(Events::GroundDropAddedEvent(entityId, snappedItem.x, snappedItem.y, snappedItem.z));

            return {};
        }

        /**
         * @brief Removes a drop item by its virtual ID.
         * @param virtualId The ID of the item to remove.
         * @return EterBase::PacketResult<void> Success or error if not found.
         */
        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override
        {
            EterBase::EntityId entityId{virtualId};

            auto it = std::find_if(m_drops.begin(), m_drops.end(),
                                   [virtualId](const GroundDropItemData& drop) { return drop.virtualId == virtualId; });

            if (it == m_drops.end())
            {
                EterBase::ModernLogger::Error("GroundDropBatchRenderer_GroundSnap: RemoveDrop failed. VID {} not found.", entityId.get());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            EterBase::ModernLogger::Debug("GroundDropBatchRenderer_GroundSnap: Removed drop VID {}", entityId.get());
            
            // Swap and pop for efficient removal
            if (it != m_drops.end() - 1)
            {
                std::swap(*it, m_drops.back());
            }
            m_drops.pop_back();

            UserInterface::Core::EventBus::GetInstance().Publish(Events::GroundDropRemovedEvent(entityId));

            return {};
        }

        /**
         * @brief Updates the drops logic over time (e.g., animations or lifetime).
         * @param deltaTime The time elapsed since the last update.
         */
        void UpdateDrops(float deltaTime) override
        {
            (void)deltaTime;
        }

        /**
         * @brief Renders all items in the batch.
         */
        void RenderInstancedDrops() override
        {
            // Implementation provided by another system or DX
        }

        /**
         * @brief Checks if a player is within range to pick up the drop.
         * @param virtualId The ID of the item.
         * @param playerX The X coordinate of the player.
         * @param playerY The Y coordinate of the player.
         * @param maxRange The maximum allowed range.
         * @return true if in range, false otherwise.
         */
        bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const override
        {
            auto it = std::find_if(m_drops.begin(), m_drops.end(),
                                   [virtualId](const GroundDropItemData& drop) { return drop.virtualId == virtualId; });

            if (it == m_drops.end())
                return false;

            float dx = it->x - playerX;
            float dy = it->y - playerY;
            float distanceSq = dx * dx + dy * dy;

            return distanceSq <= (maxRange * maxRange);
        }

        /**
         * @brief Clears all drops from the renderer.
         */
        void ClearAll() override
        {
            m_drops.clear();
            EterBase::ModernLogger::Info("GroundDropBatchRenderer_GroundSnap: Cleared all drops.");
        }

    private:
        std::vector<GroundDropItemData> m_drops;
    };

    /**
     * @brief Factory function to create the GroundSnap batch renderer.
     */
    std::unique_ptr<IGroundDropBatchRenderer> CreateGroundDropBatchRenderer_GroundSnap()
    {
        return std::make_unique<GroundDropBatchRenderer_GroundSnap>();
    }
}
