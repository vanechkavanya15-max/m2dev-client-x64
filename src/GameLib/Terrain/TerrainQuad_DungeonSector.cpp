#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>

namespace
{
    struct DungeonOcclusionEvent : public UserInterface::Core::IEvent
    {
        EterBase::DungeonId dungeonId;
        uint32_t culledPatchesCount;

        DungeonOcclusionEvent(EterBase::DungeonId id, uint32_t count)
            : dungeonId(id), culledPatchesCount(count) {}
    };
    
    struct OcclusionPortal
    {
        EterBase::EntityId portalId;
        bool isOpen;
        std::vector<uint32_t> associatedPatches;
    };
}

namespace GameLib::Terrain
{
    class DungeonSectorCuller final : public ITerrainQuadtreeCuller
    {
    public:
        explicit DungeonSectorCuller(EterBase::DungeonId dungeonId) : m_dungeonId(dungeonId)
        {
            EterBase::ModernLogger::Debug("DungeonSectorCuller initialized for DungeonId: {}", m_dungeonId.value());
        }

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            EterBase::ModernLogger::Info("Building Quadtree for Dungeon Sector X: {}, Y: {}", sectorX, sectorY);
            
            // Generate some dummy portals for this sector for demonstration purposes.
            // In a real scenario, this would be populated based on map data.
            RegisterPortal(EterBase::EntityId(100), {1, 2, 3});
            RegisterPortal(EterBase::EntityId(101), {4, 5});
            RegisterPortal(EterBase::EntityId(102), {6, 7, 8, 9});
        }
        
        void RegisterPortal(EterBase::EntityId portalId, const std::vector<uint32_t>& patches)
        {
            OcclusionPortal portal{portalId, false, patches};
            m_portals.push_back(portal);
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            EterBase::ModernLogger::Trace("Culling terrain patches for DungeonId: {}", m_dungeonId.value());

            size_t visibleCount = 0;
            
            // The logic: if a portal is closed, its associated patches are considered occluded (culled).
            // We simulate culling by iterating through portals and only rendering patches behind open portals,
            // or patches that are always visible (not associated with any portal).
            // Here, for demonstration, we will only collect patches behind *open* portals.
            
            if (outVisiblePatchIds)
            {
                for (const auto& portal : m_portals)
                {
                    if (portal.isOpen)
                    {
                        for (uint32_t patchId : portal.associatedPatches)
                        {
                            // In a full implementation, we'd also test patchId against the viewFrustum.
                            outVisiblePatchIds[visibleCount++] = patchId;
                        }
                    }
                }
            }

            uint32_t culledCount = 0;
            for (const auto& portal : m_portals)
            {
                if (!portal.isOpen)
                {
                    culledCount += portal.associatedPatches.size();
                }
            }

            DungeonOcclusionEvent event(m_dungeonId, culledCount);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return visibleCount;
        }

        uint8_t SelectLOD(float distance) const override
        {
            if (distance < 500.0f) return 0;
            if (distance < 1500.0f) return 1;
            return 2;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("Clearing DungeonSectorCuller for DungeonId: {}", m_dungeonId.value());
            m_portals.clear();
        }

        std::expected<void, EterBase::EntityError> OpenOcclusionPortal(EterBase::EntityId portalId)
        {
            if (portalId.value() == 0)
            {
                EterBase::ModernLogger::Error("Invalid occlusion portal ID for DungeonId: {}", m_dungeonId.value());
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            auto it = std::find_if(m_portals.begin(), m_portals.end(), 
                [portalId](const OcclusionPortal& portal) { return portal.portalId == portalId; });
                
            if (it == m_portals.end())
            {
                 EterBase::ModernLogger::Warning("Portal {} not found in DungeonId: {}", portalId.value(), m_dungeonId.value());
                 return std::unexpected(EterBase::EntityError::NotFound);
            }

            if (it->isOpen)
            {
                return std::unexpected(EterBase::EntityError::AlreadyExists);
            }

            it->isOpen = true;
            EterBase::ModernLogger::Info("Opened occlusion portal {} for DungeonId: {}", portalId.value(), m_dungeonId.value());
            return {};
        }

    private:
        EterBase::DungeonId m_dungeonId;
        std::vector<OcclusionPortal> m_portals;
    };

    // Factory function to expose the culler to the rest of the application
    std::unique_ptr<ITerrainQuadtreeCuller> CreateDungeonSectorCuller(EterBase::DungeonId dungeonId)
    {
        return std::make_unique<DungeonSectorCuller>(dungeonId);
    }
}
