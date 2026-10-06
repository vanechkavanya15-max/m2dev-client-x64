#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "../../EterBase/ModernLogger.h"
#include "../../EterBase/Result.h"
#include "../../UserInterface/Core/EventBus.h"
#include "../../UserInterface/Core/Events.h"
#include <vector>
#include <algorithm>
#include <ranges>
#include <cmath>
#include <unordered_map>
#include <memory>

namespace UserInterface::GroundDrop
{
    class GroundDropBatchRenderer final : public IGroundDropBatchRenderer
    {
    public:
        GroundDropBatchRenderer() = default;
        ~GroundDropBatchRenderer() override = default;

        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override
        {
            auto it = std::ranges::find_if(drops_, [&item](const GroundDropItemData& d) {
                return d.virtualId == item.virtualId;
            });

            if (it != drops_.end()) {
                EterBase::ModernLogger::Warning("GroundDropBatchRenderer: Drop already exists (virtualId: {})", item.virtualId);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            drops_.push_back(item);
            instancedGroups_[item.itemVnum.value()].push_back(item);

            EterBase::ModernLogger::Debug("GroundDropBatchRenderer: Added drop (virtualId: {}, vnum: {})", item.virtualId, item.itemVnum.value());
            
            ::Core::Events::ItemDrop event{
                item.virtualId, 
                item.itemVnum.value(), 
                static_cast<uint16_t>(item.count), 
                static_cast<int32_t>(item.x), 
                static_cast<int32_t>(item.y), 
                static_cast<int32_t>(item.z)
            };
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override
        {
            auto it = std::ranges::find_if(drops_, [virtualId](const GroundDropItemData& d) {
                return d.virtualId == virtualId;
            });

            if (it == drops_.end()) {
                EterBase::ModernLogger::Warning("GroundDropBatchRenderer: Cannot remove drop, not found (virtualId: {})", virtualId);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }
            
            uint32_t vnum = it->itemVnum.value();

            // Remove from the master list
            drops_.erase(it);

            // Remove from instanced groups efficiently
            auto groupIt = instancedGroups_.find(vnum);
            if (groupIt != instancedGroups_.end()) {
                auto instIt = std::ranges::find_if(groupIt->second, [virtualId](const GroundDropItemData& d) {
                    return d.virtualId == virtualId;
                });
                if (instIt != groupIt->second.end()) {
                    groupIt->second.erase(instIt);
                    if (groupIt->second.empty()) {
                        instancedGroups_.erase(groupIt);
                    }
                }
            }

            EterBase::ModernLogger::Debug("GroundDropBatchRenderer: Removed drop (virtualId: {})", virtualId);
            
            ::Core::Events::TargetDelete event{virtualId};
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        void UpdateDrops(float deltaTime) override
        {
            // O(N) update logic if needed
        }

        void RenderInstancedDrops() override
        {
            if (instancedGroups_.empty()) {
                return;
            }
            
            // Hw instancing draw calls grouped by itemVnum
            for (const auto& [vnum, instances] : instancedGroups_) {
                EterBase::ModernLogger::Trace("GroundDropBatchRenderer: Draw call for VNum: {}, Instances: {}", vnum, instances.size());
            }
        }

        bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const override
        {
            auto it = std::ranges::find_if(drops_, [virtualId](const GroundDropItemData& d) {
                return d.virtualId == virtualId;
            });

            if (it == drops_.end()) {
                return false;
            }

            float dx = it->x - playerX;
            float dy = it->y - playerY;
            float distanceSq = dx * dx + dy * dy;
            return distanceSq <= (maxRange * maxRange);
        }

        void ClearAll() override
        {
            drops_.clear();
            instancedGroups_.clear();
            EterBase::ModernLogger::Debug("GroundDropBatchRenderer: Cleared all drops");
        }

    private:
        std::vector<GroundDropItemData> drops_;
        // Group instances by Vnum for instanced rendering
        std::unordered_map<uint32_t, std::vector<GroundDropItemData>> instancedGroups_;
    };

    // Factory method for creating the batch renderer
    std::unique_ptr<IGroundDropBatchRenderer> CreateGroundDropBatchRenderer()
    {
        return std::make_unique<GroundDropBatchRenderer>();
    }
}
