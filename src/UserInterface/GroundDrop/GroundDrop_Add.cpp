#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "GroundDropEvents.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace UserInterface::GroundDrop
{
    class GroundDropBatchRendererAddImpl final : public IGroundDropBatchRenderer
    {
    private:
        std::vector<GroundDropItemData> m_drops;

    public:
        ~GroundDropBatchRendererAddImpl() override = default;

        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override
        {
            if (item.virtualId == 0 || item.itemVnum.value() == 0)
            {
                EterBase::ModernLogger::Error("Failed to add ground drop: invalid virtualId or itemVnum");
                return std::unexpected(EterBase::PacketError::MalformedPayload);
            }

            m_drops.push_back(item);

            EterBase::ModernLogger::Info("Added ground drop: VID={}, VNUM={}, X={}, Y={}", 
                                         item.virtualId, item.itemVnum.value(), item.x, item.y);

            UserInterface::Core::EventBus::GetInstance().Publish(Events::GroundDropAddedEvent(EterBase::EntityId(item.virtualId), item.x, item.y, item.z));

            return {};
        }

        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override
        {
            auto it = std::remove_if(m_drops.begin(), m_drops.end(),
                [virtualId](const GroundDropItemData& d) { return d.virtualId == virtualId; });
            
            if (it != m_drops.end()) {
                m_drops.erase(it, m_drops.end());
            }
            return {};
        }

        void UpdateDrops(float deltaTime) override {}

        void RenderInstancedDrops() override {}

        bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const override
        {
            return false;
        }

        void ClearAll() override 
        {
            m_drops.clear();
        }
    };

    // Factory function for external linkage
    std::unique_ptr<IGroundDropBatchRenderer> CreateGroundDropBatchRendererAddImpl()
    {
        return std::make_unique<GroundDropBatchRendererAddImpl>();
    }
}
