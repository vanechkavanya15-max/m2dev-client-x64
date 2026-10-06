#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie informujace o pomyslnym zwolnieniu zasobow quadtree terenu.
     */
    struct TerrainQuadtreeCleanupCompletedEvent : public UserInterface::Core::IEvent
    {
    };

    /**
     * @brief Implementacja czyszczenia quadtree dla strumieniowania terenu.
     */
    class TerrainQuadtree_Cleanup : public ITerrainQuadtreeCuller
    {
    public:
        TerrainQuadtree_Cleanup() = default;
        ~TerrainQuadtree_Cleanup() override = default;

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            EterBase::ModernLogger::Error("TerrainQuadtree_Cleanup::BuildQuadtree is stubbed out");
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            EterBase::ModernLogger::Error("TerrainQuadtree_Cleanup::CullTerrainPatches is stubbed out");
            return 0;
        }

        uint8_t SelectLOD(float distance) const override
        {
            EterBase::ModernLogger::Error("TerrainQuadtree_Cleanup::SelectLOD is stubbed out");
            return 0;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("TerrainQuadtree_Cleanup::Clear executed");
            
            // Emitujemy zdarzenie informujace o wyczyszczeniu quadtree
            TerrainQuadtreeCleanupCompletedEvent event;
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        /**
         * @brief Parsuje pakiet opuszczenia krainy i wykonuje zwolnienie quadtree.
         * 
         * Spelnia wymog zwracania EterBase::PacketResult<void> bez zmiany bazowego interfejsu.
         * 
         * @param buffer Bufor pakietu.
         * @return EterBase::PacketResult<void> Sukces lub blad parsowania.
         */
        EterBase::PacketResult<void> HandleTerrainLeave(std::span<const uint8_t> buffer)
        {
            if (buffer.empty())
            {
                EterBase::ModernLogger::Error("TerrainQuadtree_Cleanup::HandleTerrainLeave - Buffer underflow");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            // Oczekujemy, ze pakiet to po prostu informacja o opuszczeniu terenu
            Clear();

            return {};
        }
    };
}
