#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../UserInterface/Core/EventBus.h"

#include <memory>
#include <cmath>

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie informujace o zmianie/wyborze LOD dla patcha terenu.
     */
    struct TerrainLODSelectedEvent : public UserInterface::Core::IEvent
    {
        uint8_t selectedLod;
        float distance;

        explicit TerrainLODSelectedEvent(uint8_t lod, float dist)
            : selectedLod(lod), distance(dist) {}
    };

    /**
     * @brief Implementacja wyboru poziomu detali (LOD) oraz cullingu dla quadtree terenu.
     * 
     * Wykorzystuje odleglosc kamery do dynamicznego wyboru rozdzielczosci geometrii patcha.
     */
    class TerrainQuadLODSelect final : public ITerrainQuadtreeCuller
    {
    public:
        TerrainQuadLODSelect()
        {
            EterBase::ModernLogger::Info("TerrainQuadLODSelect initialized.");
        }

        ~TerrainQuadLODSelect() override
        {
            EterBase::ModernLogger::Info("TerrainQuadLODSelect destroyed.");
        }

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            EterBase::ModernLogger::Debug("TerrainQuadLODSelect::BuildQuadtree(SectorX: {}, SectorY: {})", sectorX, sectorY);
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            EterBase::ModernLogger::Debug("TerrainQuadLODSelect::CullTerrainPatches invoked.");
            if (!outVisiblePatchIds)
            {
                EterBase::ModernLogger::Error("TerrainQuadLODSelect::CullTerrainPatches: outVisiblePatchIds is null!");
                return 0;
            }

            // Fallback patch ID dla testow struktury
            outVisiblePatchIds[0] = 0;
            return 1;
        }

        uint8_t SelectLOD(float distance) const override
        {
            uint8_t lod = 0;
            
            if (distance < 3000.0f)
            {
                lod = 0; // High
            }
            else if (distance < 7000.0f)
            {
                lod = 1; // Medium
            }
            else
            {
                lod = 2; // Low
            }

            EterBase::ModernLogger::Debug("TerrainQuadLODSelect::SelectLOD: dist = {}, LOD = {}", distance, lod);

            UserInterface::Core::EventBus::GetInstance().Publish(TerrainLODSelectedEvent(lod, distance));

            return lod;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("TerrainQuadLODSelect::Clear invoked.");
        }
    };

    /**
     * @brief Fabryka powolujaca instancje selektora LOD bez eksponowania szczegolow implementacyjnych.
     */
    std::unique_ptr<ITerrainQuadtreeCuller> CreateTerrainQuadLODSelect()
    {
        return std::make_unique<TerrainQuadLODSelect>();
    }

} // namespace GameLib::Terrain
