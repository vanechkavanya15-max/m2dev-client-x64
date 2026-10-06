#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace GameLib::Terrain
{
    /**
     * @brief Zdarzenie oznaczajace wczytanie wysokosci wody i amplitudy fal dla danego sektora.
     */
    struct WaterHeightLoadedEvent : public UserInterface::Core::IEvent
    {
        int32_t sectorX;
        int32_t sectorY;
        float baseHeight;
        float rippleAmplitude;

        WaterHeightLoadedEvent(int32_t x, int32_t y, float h, float r)
            : sectorX(x), sectorY(y), baseHeight(h), rippleAmplitude(r) {}
    };

    /**
     * @brief Handler odpowiedzialny za inicjalizacje wysokosci wody i falowania (na styku plaz i rzek).
     */
    class MapChunkWaterHeightHandler
    {
    public:
        /**
         * @brief Laduje wysokosc wody dla danego sektora i publikuje zdarzenie do GUI/silnika.
         * 
         * @param service Referencja do uslugi strumieniowania chunkow.
         * @param coord Wspolrzedne sektora mapy.
         * @param baseHeight Podstawowa wysokosc poziomu wody.
         * @param rippleAmplitude Amplituda falowania wody.
         * @return EterBase::VoidResult<EterBase::EntityError> Sukces lub kod bledu.
         */
        static EterBase::VoidResult<EterBase::EntityError> LoadWaterHeightAndRipple(
            IMapChunkStreamingService& service,
            ChunkCoordinate coord,
            float baseHeight,
            float rippleAmplitude)
        {
            EterBase::ModernLogger::Info("Loading water height and ripple for sector ({}, {}): height={}, ripple={}",
                coord.sectorX, coord.sectorY, baseHeight, rippleAmplitude);

            if (!service.IsChunkLoaded(coord))
            {
                EterBase::ModernLogger::Debug("Chunk ({}, {}) not loaded. Requesting load...", coord.sectorX, coord.sectorY);
                service.RequestChunk(coord);
            }

            // Publikacja zdarzenia o zaladowaniu wody (Zero-Conflict)
            WaterHeightLoadedEvent event(coord.sectorX, coord.sectorY, baseHeight, rippleAmplitude);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }
    };
} // namespace GameLib::Terrain
